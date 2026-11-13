// Pruebas unitarias de TicketInventory y Logger (sin dependencias externas).
// Uso: make unit
#include <atomic>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "logger.hpp"
#include "ticket_inventory.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << "    FALLO " << __FILE__ << ':' << __LINE__ << ": " #cond \
                      << '\n';                                                   \
        }                                                                        \
    } while (0)

void run(const char* name, const std::function<void()>& test) {
    const int before = g_failures;
    test();
    std::cout << (g_failures == before ? "  [PASS] " : "  [FAIL] ") << name << '\n';
}

// Lanza `threads` hilos que ejecutan fn(i) y espera a que terminen.
void parallel(int threads, const std::function<void(int)>& fn) {
    std::vector<std::thread> pool;
    pool.reserve(static_cast<std::size_t>(threads));
    for (int i = 0; i < threads; ++i) pool.emplace_back(fn, i);
    for (auto& t : pool) t.join();
}

// ---------------------------------------------------------------- inventario
void basicPurchases(SyncMode mode) {
    TicketInventory inv(10, mode, 0);
    CHECK(inv.total() == 10);
    CHECK(inv.remaining() == 10);

    auto p = inv.purchase(4);
    CHECK(p.confirmed && p.remaining == 6);
    p = inv.purchase(7);  // insuficiente
    CHECK(!p.confirmed && p.remaining == 6);
    p = inv.purchase(6);  // exactamente lo que queda
    CHECK(p.confirmed && p.remaining == 0);
    p = inv.purchase(1);  // agotado
    CHECK(!p.confirmed && p.remaining == 0);
    CHECK(inv.remaining() == 0);
}

void invalidQuantities(SyncMode mode) {
    TicketInventory inv(5, mode, 0);
    CHECK(!inv.purchase(0).confirmed);
    CHECK(!inv.purchase(-3).confirmed);
    CHECK(inv.remaining() == 5);
}

// Muchos hilos compitiendo: nunca se vende más que el stock.
void noOversell(SyncMode mode, int threads, int attempts, int qty, int total, int windowUs) {
    TicketInventory inv(total, mode, windowUs);
    std::atomic<int> sold{0};
    std::atomic<int> confirmed{0};
    parallel(threads, [&](int) {
        for (int k = 0; k < attempts; ++k) {
            const auto p = inv.purchase(qty);
            if (p.confirmed) {
                sold += qty;
                ++confirmed;
                CHECK(p.remaining >= 0);
            }
        }
    });
    CHECK(sold.load() <= total);
    CHECK(sold.load() + inv.remaining() == total);
    // Hay demanda de sobra, así que el stock debe agotarse (salvo resto < qty).
    CHECK(inv.remaining() < qty);
    CHECK(confirmed.load() * qty == sold.load());
}

// -------------------------------------------------------------------- logger
std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    for (std::string l; std::getline(in, l);) out.push_back(l);
    return out;
}

void loggerNoInterleaving() {
    std::ostringstream sink;
    Logger log(true, false, sink);
    constexpr int kThreads = 8, kPerThread = 500;
    parallel(kThreads, [&](int i) {
        for (int j = 0; j < kPerThread; ++j) {
            log.log("T" + std::to_string(i), "mensaje " + std::to_string(j) + " fin");
        }
    });
    const auto ls = lines(sink.str());
    CHECK(ls.size() == static_cast<std::size_t>(kThreads * kPerThread));
    CHECK(log.linesWritten() == static_cast<std::size_t>(kThreads * kPerThread));
    const std::regex shape(R"(^\[T\d+\] mensaje \d+ fin$)");
    bool allWellFormed = true;
    for (const auto& l : ls) allWellFormed = allWellFormed && std::regex_match(l, shape);
    CHECK(allWellFormed);
}

void loggerQuietMode() {
    std::ostringstream sink;
    Logger log(false, false, sink);
    log.log("Worker 1", "no debe salir");
    log.always("resumen");
    CHECK(sink.str() == "resumen\n");
    CHECK(log.linesWritten() == 1);
}

void loggerTimestamps() {
    std::ostringstream sink;
    Logger log(true, true, sink);
    log.log("A", "hola");
    log.log("B", "chau");
    const auto ls = lines(sink.str());
    CHECK(ls.size() == 2);
    const std::regex shape(R"(^\[\s*(\d+\.\d{3}) ms\] \[[AB]\] (hola|chau)$)");
    std::smatch m1, m2;
    const bool ok = ls.size() == 2 && std::regex_match(ls[0], m1, shape) &&
                    std::regex_match(ls[1], m2, shape);
    CHECK(ok);
    if (ok) CHECK(std::stod(m1[1].str()) <= std::stod(m2[1].str()));  // monótono
}

void syncModeNames() {
    CHECK(std::string(toString(SyncMode::Mutex)) == "mutex");
    CHECK(std::string(toString(SyncMode::Atomic)) == "atomic");
    CHECK(std::string(toString(SyncMode::None)) == "none");
}

}  // namespace

int main() {
    std::cout << "Pruebas unitarias: TicketInventory y Logger\n";
    run("U1  compras básicas (mutex)", [] { basicPurchases(SyncMode::Mutex); });
    run("U2  compras básicas (atómico)", [] { basicPurchases(SyncMode::Atomic); });
    run("U3  cantidades inválidas se rechazan", [] {
        invalidQuantities(SyncMode::Mutex);
        invalidQuantities(SyncMode::Atomic);
    });
    run("U4  16 hilos x 1000 compras, sin sobreventa (mutex)",
        [] { noOversell(SyncMode::Mutex, 16, 1000, 1, 5000, 0); });
    run("U5  16 hilos x 1000 compras, sin sobreventa (atómico)",
        [] { noOversell(SyncMode::Atomic, 16, 1000, 1, 5000, 0); });
    run("U6  ventana de pago de 50 us, sin sobreventa (mutex y atómico)", [] {
        noOversell(SyncMode::Mutex, 8, 40, 3, 100, 50);
        noOversell(SyncMode::Atomic, 8, 40, 3, 100, 50);
    });
    run("U7  logger: 8 hilos x 500 líneas sin mezclarse", loggerNoInterleaving);
    run("U8  logger: modo quiet solo deja pasar always()", loggerQuietMode);
    run("U9  logger: marcas de tiempo con formato y orden", loggerTimestamps);
    run("U10 nombres de SyncMode", syncModeNames);

    std::cout << "\nComprobaciones: " << g_checks << ", fallidas: " << g_failures << '\n';
    return g_failures == 0 ? 0 : 1;
}
