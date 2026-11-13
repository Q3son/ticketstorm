#pragma once

#include "config.hpp"
#include "logger.hpp"
#include "request_queue.hpp"

// Hilo productor: genera peticiones aleatorias y las encola.
class Dispatcher {
public:
    Dispatcher(RequestQueue& queue,
               Logger& logger,
               const Config& config,
               unsigned seed);

    Dispatcher(const Dispatcher&)            = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    void run();  // cuerpo del hilo

    int produced() const { return produced_; }  // leer tras join()

private:
    RequestQueue&  queue_;
    Logger&        logger_;
    const Config&  config_;
    const unsigned seed_;
    int            produced_ = 0;
};
