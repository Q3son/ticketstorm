#pragma once

#include <cstddef>
#include <string>

#include "sync_mode.hpp"

// Límites del modelo de simulación (según el documento: 1 a 4 entradas).
constexpr int kMinTicketsPerRequest = 1;
constexpr int kMaxTicketsPerRequest = 4;
constexpr int kMaxUserId            = 100;
constexpr int kMaxWorkers           = 64;

// Parámetros configurables del sistema (sección 5.7 del documento).
struct Config {
    int         numWorkers    = 3;     // hilos consumidores
    std::size_t queueCapacity = 5;     // capacidad máxima de la cola
    int         totalTickets  = 50;    // stock inicial de entradas
    int         totalRequests = 30;    // peticiones que genera el Dispatcher
    int         minProcessMs  = 50;    // duración mínima del "procesamiento"
    int         maxProcessMs  = 150;   // duración máxima del "procesamiento"
    int         raceWindowUs  = 200;   // ventana entre verificar y descontar stock
    SyncMode    syncMode      = SyncMode::Mutex;  // estrategia del contador
    bool        quiet         = false; // true => solo imprime el resumen final
    bool        timestamps    = false; // true => trazas con marca de tiempo (ms)
    bool        showHelp      = false;
    unsigned    seed          = 0;     // 0 => semilla aleatoria
    std::string traceFile;             // si no está vacío, escribe la traza JSON
};

// Interpreta los argumentos de línea de comandos.
// Devuelve false y rellena `error` si algún argumento es inválido.
bool parseArgs(int argc, char** argv, Config& cfg, std::string& error);

// Imprime la ayuda de uso en la salida estándar.
void printUsage(const char* program);
