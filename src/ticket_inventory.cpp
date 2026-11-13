#include "ticket_inventory.hpp"

#include <chrono>
#include <thread>

TicketInventory::TicketInventory(int total, bool synchronized, int raceWindowUs)
    : total_(total),
      synchronized_(synchronized),
      raceWindowUs_(raceWindowUs),
      available_(total) {}

TicketInventory::Purchase TicketInventory::purchase(int quantity) {
    if (synchronized_) {
        std::lock_guard<std::mutex> lock(mtx_);  // entrada a la sección crítica
        return purchaseUnlocked(quantity);       // salida automática (RAII)
    }
    return purchaseUnlocked(quantity);           // SIN protección (versión 5.8)
}

TicketInventory::Purchase TicketInventory::purchaseUnlocked(int quantity) {
    // 1) Verificar disponibilidad.
    if (available_ < quantity) {
        return Purchase{false, available_};
    }

    // 2) Ventana de "validación de pago": ensancha el intervalo entre la
    //    verificación y el descuento. Con mutex es inofensiva; sin mutex
    //    permite que otro hilo verifique con el mismo stock (check-then-act).
    if (raceWindowUs_ > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(raceWindowUs_));
    }

    // 3) Descontar.
    available_ -= quantity;
    return Purchase{true, available_};
}

int TicketInventory::remaining() const {
    if (synchronized_) {
        std::lock_guard<std::mutex> lock(mtx_);
        return available_;
    }
    return available_;
}
