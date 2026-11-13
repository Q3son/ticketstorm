#pragma once

#include <initializer_list>
#include <string>
#include <utility>

// Registro de eventos de sincronización para el visualizador (gui/).
//
// Cada hilo se identifica con un nombre corto (D = Dispatcher, W1..WN =
// Workers). Los módulos llaman a trace::emit() en los puntos relevantes para
// el sistema operativo: espera y adquisición de mutex, wait/notify de las
// variables de condición, entrada a la sección crítica, reintentos de CAS...
//
// Cuando el trazado está desactivado (por defecto) emit() solo lee una
// bandera atómica y retorna, por lo que el costo es despreciable.
//
// El registro usa su propio mutex como cerrojo "hoja": mientras lo tiene
// nunca adquiere otro, así que no introduce riesgo de interbloqueo.
namespace trace {

using Field = std::pair<const char*, long long>;

void enable();
bool enabled();

// Nombre del hilo actual en la traza (thread_local).
void setThread(const std::string& name);

// Registra un evento. `obj` identifica el recurso (p. ej. "queue",
// "inventory", "notFull") y `fields` lleva datos numéricos opcionales.
void emit(const char* event, const char* obj = "", std::initializer_list<Field> fields = {});

// Escribe la traza completa como JSON: {"meta": <metaJson>, "events": [...],
// "summary": <summaryJson>}. Devuelve false si no pudo abrir el archivo.
bool writeJson(const std::string& path, const std::string& metaJson,
               const std::string& summaryJson);

// Cantidad de eventos registrados.
std::size_t size();

}  // namespace trace
