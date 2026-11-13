// Micro-benchmark de TicketInventory: costo por compra en modo mutex vs.
// atómico según el número de hilos. Uso: make bench
//
// Cada configuración realiza kTotalOps compras de 1 entrada repartidas entre
// los hilos, con stock suficiente para que ninguna se rechace y sin ventana
// de pago. Se informa la mediana de kReps repeticiones.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

#include "ticket_inventory.hpp"

namespace {

constexpr int kTotalOps = 2'000'000;
constexpr int kReps     = 5;

double runOnce(SyncMode mode, int threads) {
    TicketInventory inv(kTotalOps, mode, 0);
    const int perThread = kTotalOps / threads;
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    for (int i = 0; i < threads; ++i) {
        pool.emplace_back([&] {
            for (int k = 0; k < perThread; ++k) inv.purchase(1);
        });
    }
    for (auto& t : pool) t.join();
    const double ns = std::chrono::duration<double, std::nano>(
                          std::chrono::steady_clock::now() - t0).count();
    if (inv.remaining() != kTotalOps - perThread * threads) {
        std::fprintf(stderr, "ERROR: stock inconsistente en modo %s\n", toString(mode));
    }
    return ns / (perThread * threads);
}

double median(SyncMode mode, int threads) {
    std::vector<double> v;
    for (int r = 0; r < kReps; ++r) v.push_back(runOnce(mode, threads));
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

}  // namespace

int main() {
    std::printf("Benchmark TicketInventory (%d compras, mediana de %d)\n", kTotalOps, kReps);
    std::printf("hilos | mutex (ns/op) | atomic (ns/op) | atomic/mutex\n");
    for (int threads : {1, 2, 4, 8, 16}) {
        const double m = median(SyncMode::Mutex, threads);
        const double a = median(SyncMode::Atomic, threads);
        std::printf("%5d | %13.1f | %14.1f | %12.2f\n", threads, m, a, a / m);
    }
    return 0;
}
