#pragma once

#include <string>

#include "config.hpp"
#include "logger.hpp"
#include "request_queue.hpp"
#include "ticket_inventory.hpp"

// Estadísticas privadas de cada Worker (solo las toca su propio hilo y se
// leen desde main después de join(), por lo que no requieren mutex).
struct WorkerStats {
    int attended    = 0;  // peticiones atendidas
    int confirmed   = 0;  // compras confirmadas
    int rejected    = 0;  // compras rechazadas (agotado / insuficiente)
    int ticketsSold = 0;  // entradas vendidas por este Worker
};

// Hilo consumidor: toma peticiones de la cola, las procesa y descuenta stock.
class Worker {
public:
    Worker(int id,
           RequestQueue& queue,
           TicketInventory& inventory,
           Logger& logger,
           const Config& config,
           unsigned seed);

    Worker(const Worker&)            = delete;
    Worker& operator=(const Worker&) = delete;

    void run();  // cuerpo del hilo

    int                id() const { return id_; }
    const WorkerStats& stats() const { return stats_; }

private:
    const int        id_;
    RequestQueue&    queue_;
    TicketInventory& inventory_;
    Logger&          logger_;
    const Config&    config_;
    const unsigned   seed_;
    const std::string tag_;
    WorkerStats      stats_;
};
