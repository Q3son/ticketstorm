#include "logger.hpp"

#include <iostream>

void Logger::log(const std::string& tag, const std::string& message) {
    if (!enabled_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mtx_);
    std::cout << '[' << tag << "] " << message << '\n';
    std::cout.flush();
}

void Logger::always(const std::string& line) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::cout << line << '\n';
    std::cout.flush();
}
