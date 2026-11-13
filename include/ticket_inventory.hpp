#pragma once

#include <atomic>
#include <mutex>

#include "sync_mode.hpp"

// Contador compartido de entradas disponibles.
//
// La operación crítica es "verificar y luego descontar" (check-then-act).
// Según el modo de sincronización se protege de forma distinta:
//
//   * SyncMode::Mutex  : la verificación y el descuento ocurren dentro de una
//                        sección crítica protegida por un mutex propio.
//   * SyncMode::Atomic : se usa std::atomic<int> y compare_exchange_weak. Si
//                        otro hilo cambió el stock entre la lectura y la
//                        escritura, el intercambio falla y se reintenta con el
//                        valor actualizado (control de concurrencia optimista).
//   * SyncMode::None   : se omite toda protección A PROPÓSITO (extra 5.8). Es
//                        una carrera de datos deliberada (comportamiento
//                        indefinido en C++), solo con fines didácticos.
//
// Invariante (modos Mutex y Atomic): 0 <= remaining() <= total() en todo
// momento, y las entradas vendidas nunca superan el stock inicial.
class TicketInventory {
public:
    struct Purchase {
        bool confirmed;  // true si se descontaron las entradas
        int  remaining;  // stock observado tras la operación
    };

    TicketInventory(int total, SyncMode mode, int raceWindowUs);

    TicketInventory(const TicketInventory&)            = delete;
    TicketInventory& operator=(const TicketInventory&) = delete;

    // Intenta comprar `quantity` entradas. Cantidades no positivas se
    // rechazan sin modificar el stock.
    Purchase purchase(int quantity);

    // Stock actual. Seguro de consultar en cualquier momento en los modos
    // Mutex y Atomic; en modo None solo tras el join() de los Workers.
    int remaining() const;

    int      total() const { return total_; }
    SyncMode mode() const { return mode_; }
    bool     isSynchronized() const { return mode_ != SyncMode::None; }

private:
    Purchase purchaseUnlocked(int quantity);  // modos Mutex (con cerrojo) y None
    Purchase purchaseAtomic(int quantity);    // modo Atomic
    void     paymentWindow() const;           // ventana entre verificar y descontar

    const int          total_;
    const SyncMode     mode_;
    const int          raceWindowUs_;
    int                available_;        // recurso compartido (Mutex / None)
    std::atomic<int>   atomicAvailable_;  // recurso compartido (Atomic)
    mutable std::mutex mtx_;              // mutex propio del contador
};
