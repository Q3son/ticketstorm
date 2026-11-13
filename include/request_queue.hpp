#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>

#include "request.hpp"

// Cola acotada thread-safe (buffer limitado del problema productor-consumidor).
//
//   * mtx_      : exclusión mutua sobre la cola.
//   * notFull_  : condition_variable "hay espacio"  -> espera el Dispatcher.
//   * notEmpty_ : condition_variable "hay trabajo"  -> esperan los Workers.
//   * closed_   : bandera "finalizado" para terminar limpiamente.
//
// Los hooks (onBlock / onEnqueued) se ejecutan con el mutex de la cola
// tomado, para que los mensajes de consola salgan en el orden real de los
// eventos. Solo deben usar el Logger (orden de bloqueo fijo: cola -> consola).
class RequestQueue {
public:
    using BlockHook    = std::function<void()>;
    using EnqueuedHook = std::function<void(std::size_t sizeAfter)>;

    struct PushResult {
        bool        accepted;   // false si la cola fue cerrada
        bool        blocked;    // true si tuvo que esperar por falta de espacio
        std::size_t sizeAfter;  // tamaño de la cola tras la inserción
    };

    struct PopResult {
        bool got;      // false si la cola está cerrada y vacía (fin de trabajo)
        bool blocked;  // true si tuvo que esperar por cola vacía
    };

    explicit RequestQueue(std::size_t capacity);

    RequestQueue(const RequestQueue&)            = delete;
    RequestQueue& operator=(const RequestQueue&) = delete;

    // Productor: inserta; se bloquea (espera pasiva) mientras la cola esté llena.
    PushResult push(const Request& req,
                    const BlockHook& onBlock = nullptr,
                    const EnqueuedHook& onEnqueued = nullptr);

    // Consumidor: extrae; se bloquea (espera pasiva) mientras la cola esté vacía.
    PopResult pop(Request& out, const BlockHook& onBlock = nullptr);

    // Marca la cola como finalizada y despierta a todos los hilos (notify_all).
    void close();

    std::size_t capacity() const { return capacity_; }

private:
    const std::size_t        capacity_;
    std::queue<Request>      queue_;
    bool                     closed_ = false;
    std::mutex               mtx_;
    std::condition_variable  notFull_;
    std::condition_variable  notEmpty_;
};
