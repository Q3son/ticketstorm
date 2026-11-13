#pragma once

#include <mutex>
#include <string>

// Salida por consola protegida con un mutex para que los mensajes de
// distintos hilos no se mezclen (recurso compartido #3 del documento).
class Logger {
public:
    explicit Logger(bool enabled = true) : enabled_(enabled) {}

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    // Mensaje de traza "[tag] mensaje". No se imprime si el modo es quiet.
    void log(const std::string& tag, const std::string& message);

    // Línea que siempre se imprime (cabecera y resumen final).
    void always(const std::string& line);

private:
    std::mutex mtx_;
    const bool enabled_;
};
