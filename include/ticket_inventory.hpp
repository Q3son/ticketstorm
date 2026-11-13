#pragma once

#include <mutex>

// Contador compartido de entradas disponibles.
//
// Con synchronized = true la sección crítica (verificar + descontar) queda
// protegida por un mutex propio. Con synchronized = false se omite el mutex
// A PROPÓSITO (extra opcional 5.8) para mostrar la condición de carrera:
// varios hilos pueden pasar la verificación con el mismo stock y vender de más.
// Esa versión contiene una carrera de datos deliberada (comportamiento
// indefinido en C++), solo para fines didácticos; ThreadSanitizer la reporta.
class TicketInventory {
public:
    struct Purchase {
        bool confirmed;  // true si se descontaron las entradas
        int  remaining;  // stock observado tras la operación
    };

    TicketInventory(int total, bool synchronized, int raceWindowUs);

    TicketInventory(const TicketInventory&)            = delete;
    TicketInventory& operator=(const TicketInventory&) = delete;

    Purchase purchase(int quantity);

    // Stock actual. Debe consultarse cuando los hilos ya terminaron.
    int remaining() const;

    int  total() const { return total_; }
    bool isSynchronized() const { return synchronized_; }

private:
    Purchase purchaseUnlocked(int quantity);

    const int          total_;
    const bool         synchronized_;
    const int          raceWindowUs_;
    int                available_;   // recurso compartido
    mutable std::mutex mtx_;         // mutex propio del contador
};
