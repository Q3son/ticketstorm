#pragma once

// Estrategia de sincronización del contador de entradas.
//
//   Mutex  : sección crítica clásica (verificar + descontar) con std::mutex.
//   Atomic : sin cerrojos; std::atomic<int> + compare_exchange (optimista).
//   None   : SIN protección (extra 5.8). Contiene una carrera de datos
//            deliberada, solo para demostrar la sobreventa.
enum class SyncMode { Mutex, Atomic, None };

// Nombre corto del modo ("mutex", "atomic", "none").
const char* toString(SyncMode mode);

// Descripción legible para la cabecera de la simulación.
const char* describe(SyncMode mode);
