# TicketStorm

Simulación concurrente de un sistema de venta de entradas en **C++17** que implementa el
problema **productor-consumidor** con una cola acotada, `std::mutex` y `std::condition_variable`.

- **Dispatcher** (productor): genera peticiones aleatorias (usuario y 1 a 4 entradas).
- **Workers** (consumidores): procesan las peticiones y descuentan entradas del inventario.
- **RequestQueue**: buffer acotado con condiciones «hay espacio» / «hay trabajo» y bandera de cierre.
- **TicketInventory**: contador protegido por mutex; con `--no-sync` se omite para demostrar la condición de carrera.
- **Logger**: salida por consola protegida con mutex.

## Compilar y probar

Requisitos: compilador con C++17 e hilos (g++/clang++), GNU Make y Bash.
Probado en Windows 11 con MSYS2 (g++ 16.1, UCRT64); también compila en Linux y macOS.

```bash
make                      # compila ./ticketstorm
make test                 # plan de pruebas T1–T8
./ticketstorm --seed 42   # ejecución reproducible
./ticketstorm --help      # todas las opciones
make tsan                 # ThreadSanitizer (Linux/macOS)
```

| Código de salida | Significado |
|---|---|
| 0 | Verificación correcta |
| 1 | Condición de carrera detectada (sobreventa o stock inconsistente) |
| 2 | Peticiones perdidas |
| 64 | Argumentos inválidos |

## Estructura

```
include/   cabeceras (config, dispatcher, logger, request, request_queue, simulation, ticket_inventory, worker)
src/       implementación y main.cpp
scripts/   run_tests.sh (plan de pruebas)
docs/      informe técnico (DOCX y PDF)
```

## Equipo

| Integrante | Módulos |
|---|---|
| Mejía Rondan, Giovanni Patrick | `request_queue`, `request` |
| Guillen Valcarcel, Daniel Josue | `dispatcher` |
| Cari Quispe, Juan Manuel | `worker` |
| Subia Huaicane, Edson Fabricio | `ticket_inventory`, `logger` |
| Mengoa Valeriano, Brigitte Noelia | `config`, `simulation`, `main`, `Makefile`, `run_tests.sh` |

El informe completo está en [`docs/Informe_TicketStorm.pdf`](docs/Informe_TicketStorm.pdf).
