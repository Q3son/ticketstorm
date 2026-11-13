#pragma once

#include <chrono>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <ostream>
#include <string>

// Salida por consola protegida con un mutex para que los mensajes de
// distintos hilos no se mezclen (recurso compartido #3 del documento).
//
// Mejoras respecto a la versión inicial:
//   * Flujo de salida inyectable (por defecto std::cout), lo que permite
//     probar el Logger escribiendo en un std::ostringstream.
//   * Marcas de tiempo opcionales relativas al inicio de la simulación,
//     tomadas DENTRO de la sección crítica para que sean monótonas en el
//     mismo orden en que aparecen las líneas.
//   * Contador de líneas escritas, consultable de forma segura.
//
// Orden de bloqueo: el Logger puede llamarse con el mutex de la cola tomado
// (cola -> consola), pero nunca adquiere otro cerrojo mientras tiene el suyo.
class Logger {
public:
    explicit Logger(bool enabled = true,
                    bool timestamps = false,
                    std::ostream& out = std::cout);

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    // Mensaje de traza "[tag] mensaje". No se imprime si el modo es quiet.
    void log(const std::string& tag, const std::string& message);

    // Línea que siempre se imprime (cabecera y resumen final), sin marca de tiempo.
    void always(const std::string& line);

    // Cantidad de escrituras realizadas (log + always).
    std::size_t linesWritten() const;

    bool enabled() const { return enabled_; }
    bool timestamps() const { return timestamps_; }

private:
    std::string timestampLocked() const;  // requiere mtx_ tomado

    using Clock = std::chrono::steady_clock;

    mutable std::mutex      mtx_;
    std::ostream&           out_;
    const bool              enabled_;
    const bool              timestamps_;
    const Clock::time_point start_;
    std::size_t             lines_ = 0;
};
