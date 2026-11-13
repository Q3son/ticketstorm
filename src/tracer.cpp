#include "tracer.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <vector>

namespace trace {
namespace {

struct Event {
    long long                                 tUs;
    std::string                               thread;
    std::string                               event;
    std::string                               obj;
    std::vector<std::pair<std::string, long long>> fields;
};

using Clock = std::chrono::steady_clock;

std::atomic<bool>  g_enabled{false};
std::mutex         g_mtx;  // cerrojo hoja
std::vector<Event> g_events;
Clock::time_point  g_start = Clock::now();
thread_local std::string t_name = "main";

void writeEscaped(std::ofstream& out, const std::string& s) {
    out << '"';
    for (const char c : s) {
        if (c == '"' || c == '\\') out << '\\';
        out << c;
    }
    out << '"';
}

}  // namespace

void enable() {
    std::lock_guard<std::mutex> lock(g_mtx);
    g_events.clear();
    g_events.reserve(4096);
    g_start = Clock::now();
    g_enabled.store(true, std::memory_order_release);
}

bool enabled() { return g_enabled.load(std::memory_order_acquire); }

void setThread(const std::string& name) { t_name = name; }

void emit(const char* event, const char* obj, std::initializer_list<Field> fields) {
    if (!enabled()) {
        return;
    }
    Event e;
    e.thread = t_name;
    e.event  = event;
    e.obj    = obj;
    for (const auto& f : fields) e.fields.emplace_back(f.first, f.second);

    std::lock_guard<std::mutex> lock(g_mtx);
    // La marca de tiempo se toma con el cerrojo: el orden del vector coincide
    // con el orden temporal.
    e.tUs = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - g_start).count();
    g_events.push_back(std::move(e));
}

std::size_t size() {
    std::lock_guard<std::mutex> lock(g_mtx);
    return g_events.size();
}

bool writeJson(const std::string& path, const std::string& metaJson,
               const std::string& summaryJson) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_mtx);
    out << "{\"meta\":" << metaJson << ",\"summary\":" << summaryJson << ",\"events\":[\n";
    for (std::size_t i = 0; i < g_events.size(); ++i) {
        const Event& e = g_events[i];
        out << "{\"t\":" << e.tUs << ",\"th\":";
        writeEscaped(out, e.thread);
        out << ",\"ev\":";
        writeEscaped(out, e.event);
        if (!e.obj.empty()) {
            out << ",\"obj\":";
            writeEscaped(out, e.obj);
        }
        for (const auto& f : e.fields) {
            out << ",\"" << f.first << "\":" << f.second;
        }
        out << '}' << (i + 1 < g_events.size() ? ",\n" : "\n");
    }
    out << "]}\n";
    return static_cast<bool>(out);
}

}  // namespace trace
