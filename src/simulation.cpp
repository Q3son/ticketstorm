#include "simulation.hpp"

#include <chrono>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "dispatcher.hpp"
#include "logger.hpp"
#include "request_queue.hpp"
#include "ticket_inventory.hpp"
#include "tracer.hpp"
#include "worker.hpp"

int Simulation::run() {
    const unsigned baseSeed =
        config_.seed != 0 ? config_.seed : std::random_device{}();

    Logger          logger(!config_.quiet, config_.timestamps);
    RequestQueue    queue(config_.queueCapacity);
    TicketInventory inventory(config_.totalTickets, config_.syncMode,
                              config_.raceWindowUs);

    {
        std::ostringstream os;
        os << "=== TicketStorm ===\n"
           << "Workers: " << config_.numWorkers
           << " | Capacidad de cola: " << config_.queueCapacity
           << " | Entradas: " << config_.totalTickets
           << " | Peticiones: " << config_.totalRequests << '\n'
           << "Modo: " << describe(config_.syncMode)
           << " | Semilla: " << baseSeed << '\n';
        logger.always(os.str());
    }

    std::vector<std::unique_ptr<Worker>> workers;
    workers.reserve(static_cast<std::size_t>(config_.numWorkers));
    for (int i = 0; i < config_.numWorkers; ++i) {
        workers.push_back(std::make_unique<Worker>(
            i + 1, queue, inventory, logger, config_,
            baseSeed + 1000u + static_cast<unsigned>(i)));
    }
    Dispatcher dispatcher(queue, logger, config_, baseSeed);

    if (!config_.traceFile.empty()) {
        trace::enable();
        trace::setThread("main");
    }

    const auto start = std::chrono::steady_clock::now();

    // Se crean primero los Workers y luego el Dispatcher (flujo 5.5, paso 1).
    std::vector<std::thread> workerThreads;
    workerThreads.reserve(workers.size());
    for (auto& w : workers) {
        Worker* worker = w.get();
        workerThreads.emplace_back([worker] { worker->run(); });
    }
    std::thread dispatcherThread([&dispatcher] { dispatcher.run(); });

    dispatcherThread.join();
    for (auto& t : workerThreads) {
        t.join();
    }

    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();

    // ---- Resumen final ----------------------------------------------------
    int attended = 0, rejected = 0, sold = 0;
    for (const auto& w : workers) {
        attended += w->stats().attended;
        rejected += w->stats().rejected;
        sold += w->stats().ticketsSold;
    }
    const int remaining = inventory.remaining();
    const int total     = inventory.total();

    std::ostringstream os;
    os << "\n=== RESUMEN ===\n";
    for (const auto& w : workers) {
        const WorkerStats& s = w->stats();
        os << "Worker " << w->id() << ": " << s.attended << " peticiones"
           << " (confirmadas: " << s.confirmed << ", rechazadas: " << s.rejected
           << ", entradas vendidas: " << s.ticketsSold << ")\n";
    }
    os << "Peticiones generadas: " << dispatcher.produced()
       << " | Peticiones atendidas: " << attended << '\n'
       << "Entradas vendidas: " << sold << " | Peticiones rechazadas: " << rejected
       << '\n'
       << "Entradas restantes: " << remaining << " (stock inicial: " << total << ")\n"
       << "Tiempo total: " << elapsedMs << " ms\n";

    int exitCode = 0;
    if (sold > total || remaining < 0) {
        os << "Verificación: ¡CONDICIÓN DE CARRERA DETECTADA! Se vendieron " << sold
           << " entradas habiendo solo " << total << " disponibles.";
        exitCode = 1;
    } else if (sold + remaining != total) {
        os << "Verificación: ¡CONDICIÓN DE CARRERA DETECTADA! Inconsistencia: vendidas ("
           << sold << ") + restantes (" << remaining << ") != stock inicial (" << total
           << ").";
        exitCode = 1;
    } else if (attended != dispatcher.produced()) {
        os << "Verificación: ERROR, se perdieron peticiones (" << attended << " de "
           << dispatcher.produced() << ").";
        exitCode = 2;
    } else {
        os << "Verificación: OK (vendidas + restantes = stock inicial; las entradas "
              "vendidas nunca superan las disponibles).";
    }
    logger.always(os.str());

    if (!config_.traceFile.empty()) {
        std::ostringstream meta, summary;
        meta << "{\"workers\":" << config_.numWorkers << ",\"capacity\":" << config_.queueCapacity
             << ",\"tickets\":" << total << ",\"requests\":" << config_.totalRequests
             << ",\"mode\":\"" << toString(config_.syncMode) << "\",\"seed\":" << baseSeed
             << ",\"minMs\":" << config_.minProcessMs << ",\"maxMs\":" << config_.maxProcessMs
             << ",\"windowUs\":" << config_.raceWindowUs << '}';
        summary << "{\"generated\":" << dispatcher.produced() << ",\"attended\":" << attended
                << ",\"sold\":" << sold << ",\"rejected\":" << rejected
                << ",\"remaining\":" << remaining << ",\"total\":" << total
                << ",\"elapsedMs\":" << elapsedMs << ",\"exitCode\":" << exitCode << '}';
        if (trace::writeJson(config_.traceFile, meta.str(), summary.str())) {
            logger.always("Traza: " + std::to_string(trace::size()) + " eventos guardados en " +
                          config_.traceFile);
        } else {
            logger.always("Error: no se pudo escribir la traza en " + config_.traceFile);
        }
    }

    return exitCode;
}
