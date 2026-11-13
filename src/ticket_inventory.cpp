#include "ticket_inventory.hpp"

#include <chrono>
#include <thread>

const char* toString(SyncMode mode) {
    switch (mode) {
        case SyncMode::Mutex:  return "mutex";
        case SyncMode::Atomic: return "atomic";
        case SyncMode::None:   return "none";
    }
    return "?";
}

const char* describe(SyncMode mode) {
    switch (mode) {
        case SyncMode::Mutex:  return "CON sincronización (mutex)";
        case SyncMode::Atomic: return "CON sincronización (atómica, compare_exchange)";
        case SyncMode::None:   return "SIN sincronización (mutex omitido)";
    }
    return "?";
}

TicketInventory::TicketInventory(int total, SyncMode mode, int raceWindowUs)
    : total_(total < 0 ? 0 : total),
      mode_(mode),
      raceWindowUs_(raceWindowUs < 0 ? 0 : raceWindowUs),
      available_(total_),
      atomicAvailable_(total_) {}

TicketInventory::Purchase TicketInventory::purchase(int quantity) {
    if (quantity <= 0) {
        return Purchase{false, remaining()};  // petición inválida: no toca el stock
    }
    switch (mode_) {
        case SyncMode::Mutex: {
            std::lock_guard<std::mutex> lock(mtx_);  // entrada a la sección crítica
            return purchaseUnlocked(quantity);       // salida automática (RAII)
        }
        case SyncMode::Atomic:
            return purchaseAtomic(quantity);
        case SyncMode::None:
            break;
    }
    return purchaseUnlocked(quantity);  // SIN protección (versión 5.8)
}

void TicketInventory::paymentWindow() const {
    // Ventana de "validación de pago": ensancha el intervalo entre la
    // verificación y el descuento. Con mutex o atómicos es inofensiva; sin
    // protección permite que otro hilo verifique con el mismo stock.
    if (raceWindowUs_ > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(raceWindowUs_));
    }
}

TicketInventory::Purchase TicketInventory::purchaseUnlocked(int quantity) {
    // 1) Verificar disponibilidad.
    if (available_ < quantity) {
        return Purchase{false, available_};
    }
    // 2) Validación de pago.
    paymentWindow();
    // 3) Descontar.
    available_ -= quantity;
    return Purchase{true, available_};
}

TicketInventory::Purchase TicketInventory::purchaseAtomic(int quantity) {
    int current = atomicAvailable_.load(std::memory_order_acquire);
    while (true) {
        // 1) Verificar sobre la copia leída.
        if (current < quantity) {
            return Purchase{false, current};
        }
        // 2) Validación de pago (sin cerrojo: otros hilos siguen avanzando).
        paymentWindow();
        // 3) Descontar solo si nadie cambió el stock desde la lectura. Si
        //    falló, `current` recibe el valor actual y se vuelve a verificar.
        if (atomicAvailable_.compare_exchange_weak(current, current - quantity,
                                                   std::memory_order_acq_rel,
                                                   std::memory_order_acquire)) {
            return Purchase{true, current - quantity};
        }
    }
}

int TicketInventory::remaining() const {
    switch (mode_) {
        case SyncMode::Mutex: {
            std::lock_guard<std::mutex> lock(mtx_);
            return available_;
        }
        case SyncMode::Atomic:
            return atomicAvailable_.load(std::memory_order_acquire);
        case SyncMode::None:
            break;
    }
    return available_;
}
