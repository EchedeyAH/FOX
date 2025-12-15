#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace common {

inline std::string TimestampToString()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    const auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%F %T");
    return oss.str();
}

template<typename T>
inline std::string to_hex_string(T value)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase << value;
    return ss.str();
}

inline void Log(const std::string &level, const std::string &module, const std::string &message)
{
    std::cerr << '[' << TimestampToString() << "] [" << level << "] [" << module << "] " << message << std::endl;
}

#define LOG_INFO(module, message) ::common::Log("INFO", module, message)
#define LOG_WARN(module, message) ::common::Log("WARN", module, message)
#define LOG_ERROR(module, message) ::common::Log("ERROR", module, message)
#define LOG_DEBUG(module, message) ::common::Log("DEBUG", module, message)

} // namespace common
