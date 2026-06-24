#include "connected_vehicle_platform/logger.hpp"

#include <chrono>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

namespace cvp {
namespace {

const char* levelToString(LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return "TRACE";
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warning:
      return "WARNING";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Fatal:
      return "FATAL";
  }
  return "INFO";
}

}  // namespace

Logger::Logger(std::string log_path) : log_path_(std::move(log_path)) {
  const auto parent = std::filesystem::path(log_path_).parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
}

void Logger::log(LogLevel level, std::string_view message,
                 std::source_location location) const {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  localtime_r(&time, &tm);

  std::ostringstream line;
  line << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [" << levelToString(level) << "] "
       << message << " @ " << location.file_name() << ':' << location.line();

  std::scoped_lock lock(mutex_);
  std::cout << line.str() << '\n';
  std::ofstream file(log_path_, std::ios::app);
  if (file.is_open()) {
    file << line.str() << '\n';
  }
}

void Logger::trace(std::string_view message, std::source_location location) const {
  log(LogLevel::Trace, message, location);
}

void Logger::debug(std::string_view message, std::source_location location) const {
  log(LogLevel::Debug, message, location);
}

void Logger::info(std::string_view message, std::source_location location) const {
  log(LogLevel::Info, message, location);
}

void Logger::warning(std::string_view message, std::source_location location) const {
  log(LogLevel::Warning, message, location);
}

void Logger::error(std::string_view message, std::source_location location) const {
  log(LogLevel::Error, message, location);
}

void Logger::fatal(std::string_view message, std::source_location location) const {
  log(LogLevel::Fatal, message, location);
}

}  // namespace cvp
