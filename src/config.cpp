#include "config.hpp"

#include <iostream>
#include <stdexcept>

namespace {

// Convierte `text` a entero validando que sea numérico y esté en [lo, hi].
// Se usa long long (64 bits garantizados): en Windows (modelo LLP64) `long`
// tiene solo 32 bits y no puede representar el máximo de --seed (2^32 - 1).
bool parseInt(const std::string& option, const std::string& text, long long lo,
              long long hi, long long& out, std::string& error) {
    try {
        std::size_t pos = 0;
        const long long value = std::stoll(text, &pos);
        if (pos != text.size()) {
            throw std::invalid_argument("resto");
        }
        if (value < lo || value > hi) {
            error = "El valor de " + option + " debe estar entre " +
                    std::to_string(lo) + " y " + std::to_string(hi) + ".";
            return false;
        }
        out = value;
        return true;
    } catch (const std::exception&) {
        error = "Valor inválido para " + option + ": '" + text + "'.";
        return false;
    }
}

}  // namespace

bool parseArgs(int argc, char** argv, Config& cfg, std::string& error) {
    bool modeGiven = false;  // detecta modos de sincronización contradictorios
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            cfg.showHelp = true;
            return true;
        }
        if (arg == "--no-sync" || arg == "--atomic") {
            const SyncMode mode = arg == "--atomic" ? SyncMode::Atomic : SyncMode::None;
            if (modeGiven && cfg.syncMode != mode) {
                error = "Las opciones --no-sync, --atomic y --sync-mode son excluyentes.";
                return false;
            }
            cfg.syncMode = mode;
            modeGiven = true;
            continue;
        }
        if (arg == "--quiet") {
            cfg.quiet = true;
            continue;
        }
        if (arg == "--timestamps") {
            cfg.timestamps = true;
            continue;
        }

        // Opciones con valor.
        const bool withValue = arg == "-w" || arg == "--workers" || arg == "-q" ||
                               arg == "--queue" || arg == "-t" || arg == "--tickets" ||
                               arg == "-r" || arg == "--requests" || arg == "--min-ms" ||
                               arg == "--max-ms" || arg == "--window-us" ||
                               arg == "--seed" || arg == "--sync-mode" ||
                               arg == "--trace";
        if (!withValue) {
            error = "Opción desconocida: " + arg;
            return false;
        }
        if (i + 1 >= argc) {
            error = "Falta el valor de la opción " + arg + ".";
            return false;
        }

        const std::string value = argv[++i];
        long long n = 0;

        if (arg == "--trace") {
            if (value.empty()) {
                error = "La opción --trace necesita un nombre de archivo.";
                return false;
            }
            cfg.traceFile = value;
        } else if (arg == "--sync-mode") {
            SyncMode mode;
            if (value == "mutex") {
                mode = SyncMode::Mutex;
            } else if (value == "atomic") {
                mode = SyncMode::Atomic;
            } else if (value == "none") {
                mode = SyncMode::None;
            } else {
                error = "Valor inválido para --sync-mode: '" + value +
                        "' (use mutex, atomic o none).";
                return false;
            }
            if (modeGiven && cfg.syncMode != mode) {
                error = "Las opciones --no-sync, --atomic y --sync-mode son excluyentes.";
                return false;
            }
            cfg.syncMode = mode;
            modeGiven = true;
        } else if (arg == "-w" || arg == "--workers") {
            if (!parseInt(arg, value, 1, kMaxWorkers, n, error)) return false;
            cfg.numWorkers = static_cast<int>(n);
        } else if (arg == "-q" || arg == "--queue") {
            if (!parseInt(arg, value, 1, 100000, n, error)) return false;
            cfg.queueCapacity = static_cast<std::size_t>(n);
        } else if (arg == "-t" || arg == "--tickets") {
            if (!parseInt(arg, value, 1, 10000000, n, error)) return false;
            cfg.totalTickets = static_cast<int>(n);
        } else if (arg == "-r" || arg == "--requests") {
            if (!parseInt(arg, value, 1, 10000000, n, error)) return false;
            cfg.totalRequests = static_cast<int>(n);
        } else if (arg == "--min-ms") {
            if (!parseInt(arg, value, 0, 60000, n, error)) return false;
            cfg.minProcessMs = static_cast<int>(n);
        } else if (arg == "--max-ms") {
            if (!parseInt(arg, value, 0, 60000, n, error)) return false;
            cfg.maxProcessMs = static_cast<int>(n);
        } else if (arg == "--window-us") {
            if (!parseInt(arg, value, 0, 1000000, n, error)) return false;
            cfg.raceWindowUs = static_cast<int>(n);
        } else {  // --seed
            if (!parseInt(arg, value, 0, 4294967295LL, n, error)) return false;
            cfg.seed = static_cast<unsigned>(n);
        }
    }

    if (cfg.minProcessMs > cfg.maxProcessMs) {
        error = "--min-ms no puede ser mayor que --max-ms.";
        return false;
    }
    return true;
}

void printUsage(const char* program) {
    std::cout
        << "Uso: " << program << " [opciones]\n\n"
        << "Opciones:\n"
        << "  -w, --workers N     Número de Workers (1-" << kMaxWorkers << ")      [3]\n"
        << "  -q, --queue N       Capacidad de la cola                   [5]\n"
        << "  -t, --tickets N     Entradas disponibles                   [50]\n"
        << "  -r, --requests N    Total de peticiones a generar          [30]\n"
        << "      --min-ms N      Procesamiento mínimo por petición (ms) [50]\n"
        << "      --max-ms N      Procesamiento máximo por petición (ms) [150]\n"
        << "      --window-us N   Ventana verificar->descontar (us)      [200]\n"
        << "      --sync-mode M   Sincronización del contador: mutex | atomic | none [mutex]\n"
        << "      --atomic        Atajo de --sync-mode atomic (compare_exchange, sin cerrojo)\n"
        << "      --no-sync       Atajo de --sync-mode none (condición de carrera)\n"
        << "      --quiet         Solo imprime cabecera y resumen final\n"
        << "      --timestamps    Antepone a cada traza el tiempo transcurrido (ms)\n"
        << "      --trace ARCHIVO Guarda los eventos de sincronización en JSON (gui/)\n"
        << "      --seed N        Semilla de las peticiones (0 = aleatoria) [0]\n"
        << "  -h, --help          Muestra esta ayuda\n\n"
        << "Código de salida: 0 = OK, 1 = condición de carrera detectada, "
           "2 = peticiones perdidas, 64 = argumentos inválidos.\n";
}
