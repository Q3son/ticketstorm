#include "dispatcher.hpp"

#include <cstddef>
#include <random>
#include <string>

#include "tracer.hpp"

namespace {
const char* const kTag = "Dispatcher";
}

Dispatcher::Dispatcher(RequestQueue& queue,
                       Logger& logger,
                       const Config& config,
                       unsigned seed)
    : queue_(queue), logger_(logger), config_(config), seed_(seed) {}

void Dispatcher::run() {
    std::mt19937 rng(seed_);
    std::uniform_int_distribution<int> userDist(1, kMaxUserId);
    std::uniform_int_distribution<int> ticketDist(kMinTicketsPerRequest,
                                                  kMaxTicketsPerRequest);
    const std::size_t capacity = queue_.capacity();
    trace::setThread("D");
    trace::emit("start");

    for (int i = 1; i <= config_.totalRequests; ++i) {
        Request req;
        req.id      = i;
        req.userId  = userDist(rng);
        req.tickets = ticketDist(rng);
        trace::emit("gen", "", {{"id", req.id}, {"user", req.userId}, {"qty", req.tickets}});

        const RequestQueue::PushResult res = queue_.push(
            req,
            // Se ejecuta justo antes de bloquearse (cola llena).
            [this] { logger_.log(kTag, "Cola llena, esperando espacio..."); },
            // Se ejecuta con la petición ya encolada (mismo orden que los eventos).
            [this, &req, capacity](std::size_t sizeAfter) {
                logger_.log(kTag, "Petición #" + std::to_string(req.id) +
                                      ": Usuario " + std::to_string(req.userId) +
                                      " solicita " + std::to_string(req.tickets) +
                                      " entradas (cola: " + std::to_string(sizeAfter) +
                                      "/" + std::to_string(capacity) + ")");
            });

        if (!res.accepted) {
            logger_.log(kTag, "Cola cerrada; se detiene la generación.");
            break;
        }
        ++produced_;
        if (res.blocked) {
            logger_.log(kTag, "Espacio liberado, se reanuda la generación.");
        }
    }

    logger_.log(kTag, "Todas las peticiones fueron generadas. Activando bandera de "
                      "finalizado y despertando a los Workers.");
    queue_.close();  // bandera "finalizado" + notify_all
    trace::emit("end");
}
