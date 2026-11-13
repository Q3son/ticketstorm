#include "ticket_inventory.hpp"

#include <chrono>
#include <thread>

#include "tracer.hpp"

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
            // Entrada a la sección crítica (se registra si hubo que esperar).
            std::unique_lock<std::mutex> lock(mtx_, std::try_to_lock);
            if (!lock.owns_lock()) {
                trace::emit("lock_wait", "inventory");
                lock.lock();
            }
            trace::emit("lock", "inventory");
            const Purchase result = purchaseUnlocked(quantity);
            trace::emit("unlock", "inventory");
            return result;  // salida automática (RAII)
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
    const int seen = available_;
    trace::emit("check", "inventory", {{"avail", seen}, {"qty", quantity}});
    if (seen < quantity) {
        trace::emit("reject", "inventory", {{"avail", seen}, {"qty", quantity}});
        return Purchase{false, seen};
    }
    // 2) Validación de pago.
    paymentWindow();
    // 3) Descontar.
    available_ -= quantity;
    const int after = available_;
    trace::emit("commit", "inventory", {{"qty", quantity}, {"remaining", after}});
    return Purchase{true, after};
}

TicketInventory::Purchase TicketInventory::purchaseAtomic(int quantity) {
    int current = atomicAvailable_.load(std::memory_order_acquire);
    while (true) {
        // 1) Verificar sobre la copia leída.
        trace::emit("check", "inventory", {{"avail", current}, {"qty", quantity}});
        if (current < quantity) {
            trace::emit("reject", "inventory", {{"avail", current}, {"qty", quantity}});
            return Purchase{false, current};
        }
        // 2) Validación de pago (sin cerrojo: otros hilos siguen avanzando).
        paymentWindow();
        // 3) Descontar solo si nadie cambió el stock desde la lectura. Si
        //    falló, `current` recibe el valor actual y se vuelve a verificar.
        if (atomicAvailable_.compare_exchange_weak(current, current - quantity,
                                                   std::memory_order_acq_rel,
                                                   std::memory_order_acquire)) {
            trace::emit("commit", "inventory", {{"qty", quantity}, {"remaining", current - quantity}});
            return Purchase{true, current - quantity};
        }
        // Otro hilo modificó el stock: el CAS falló y `current` ya trae el valor nuevo.
        trace::emit("cas_fail", "inventory", {{"seen", current}});
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
