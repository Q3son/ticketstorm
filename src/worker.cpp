#include "worker.hpp"

#include <chrono>
#include <random>
#include <thread>

#include "tracer.hpp"

Worker::Worker(int id,
               RequestQueue& queue,
               TicketInventory& inventory,
               Logger& logger,
               const Config& config,
               unsigned seed)
    : id_(id),
      queue_(queue),
      inventory_(inventory),
      logger_(logger),
      config_(config),
      seed_(seed),
      tag_("Worker " + std::to_string(id)) {}

void Worker::run() {
    std::mt19937 rng(seed_);
    std::uniform_int_distribution<int> processDist(config_.minProcessMs,
                                                   config_.maxProcessMs);
    Request req;
    trace::setThread("W" + std::to_string(id_));
    trace::emit("start");

    while (true) {
        const RequestQueue::PopResult res = queue_.pop(req, [this] {
            logger_.log(tag_, "Cola vacía, esperando trabajo...");
        });

        if (!res.got) {
            break;  // cola cerrada y vacía: terminación limpia
        }
        if (res.blocked) {
            logger_.log(tag_, "Despertado: hay trabajo disponible.");
        }

        ++stats_.attended;
        logger_.log(tag_, "Atendiendo petición #" + std::to_string(req.id) + "...");

        // Simula el tiempo de procesamiento (el hilo duerme: no usa CPU).
        const int processMs = processDist(rng);
        trace::emit("proc_start", "", {{"id", req.id}, {"ms", processMs}});
        std::this_thread::sleep_for(std::chrono::milliseconds(processMs));
        trace::emit("proc_end", "", {{"id", req.id}});

        trace::emit("buy", "", {{"id", req.id}, {"qty", req.tickets}, {"user", req.userId}});
        const TicketInventory::Purchase purchase = inventory_.purchase(req.tickets);
        trace::emit("result", "", {{"id", req.id}, {"ok", purchase.confirmed ? 1 : 0},
                                   {"qty", req.tickets}, {"remaining", purchase.remaining}});

        if (purchase.confirmed) {
            ++stats_.confirmed;
            stats_.ticketsSold += req.tickets;
            logger_.log(tag_, "Compra confirmada (Petición #" + std::to_string(req.id) +
                                  ", Usuario " + std::to_string(req.userId) + ", " +
                                  std::to_string(req.tickets) +
                                  " entradas). Entradas restantes: " +
                                  std::to_string(purchase.remaining));
        } else {
            ++stats_.rejected;
            logger_.log(tag_, "Petición #" + std::to_string(req.id) +
                                  ": entradas agotadas para Usuario " +
                                  std::to_string(req.userId) + " (pidió " +
                                  std::to_string(req.tickets) + ", quedan " +
                                  std::to_string(purchase.remaining) + ")");
        }
    }

    logger_.log(tag_, "Cola cerrada y vacía. Terminando.");
    trace::emit("end");
}
