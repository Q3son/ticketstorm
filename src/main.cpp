#include <iostream>
#include <string>

#include "config.hpp"
#include "simulation.hpp"

int main(int argc, char** argv) {
    Config cfg;
    std::string error;

    if (!parseArgs(argc, argv, cfg, error)) {
        std::cerr << "Error: " << error << "\n\n";
        printUsage(argv[0]);
        return 64;
    }
    if (cfg.showHelp) {
        printUsage(argv[0]);
        return 0;
    }

    Simulation simulation(cfg);
    return simulation.run();
}
