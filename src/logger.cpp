#include "logger.hpp"

#include <cstdio>

Logger::Logger(bool enabled, bool timestamps, std::ostream& out)
    : out_(out), enabled_(enabled), timestamps_(timestamps), start_(Clock::now()) {}

std::string Logger::timestampLocked() const {
    const double ms =
        std::chrono::duration<double, std::milli>(Clock::now() - start_).count();
    char buf[32];
    std::snprintf(buf, sizeof(buf), "[%10.3f ms] ", ms);
    return buf;
}

void Logger::log(const std::string& tag, const std::string& message) {
    if (!enabled_) {
        return;
    }
    // El texto se arma fuera de la sección crítica para acortarla.
    const std::string body = '[' + tag + "] " + message + '\n';

    std::lock_guard<std::mutex> lock(mtx_);
    if (timestamps_) {
        out_ << timestampLocked();
    }
    out_ << body;
    out_.flush();
    ++lines_;
}

void Logger::always(const std::string& line) {
    std::lock_guard<std::mutex> lock(mtx_);
    out_ << line << '\n';
    out_.flush();
    ++lines_;
}

std::size_t Logger::linesWritten() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return lines_;
}
