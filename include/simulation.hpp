#pragma once

#include "config.hpp"

// Orquesta la simulación completa: crea los hilos, espera su fin e imprime
// el resumen final.
//
// Código de retorno de run():
//   0 -> ejecución correcta
//   1 -> se detectó una condición de carrera (vendidas > disponibles o
//        vendidas + restantes != stock inicial)
//   2 -> se perdieron peticiones (atendidas != generadas)
class Simulation {
public:
    explicit Simulation(const Config& config) : config_(config) {}

    int run();

private:
    Config config_;
};
