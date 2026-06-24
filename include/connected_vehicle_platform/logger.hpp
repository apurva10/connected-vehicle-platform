#pragma once

#include <mutex>
#include <source_location>
#include <string>
#include <string_view>

namespace cvp {

enum class LogLevel {
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  Fatal
};

class Logger {
 public:
  explicit Logger(std::string log_path = "logs/vehicle.log");

  void log(LogLevel level, std::string_view message,
           std::source_location location = std::source_location::current()) const;

  void trace(std::string_view message,
             std::source_location location = std::source_location::current()) const;
  void debug(std::string_view message,
             std::source_location location = std::source_location::current()) const;
  void info(std::string_view message,
            std::source_location location = std::source_location::current()) const;
  void warning(std::string_view message,
               std::source_location location = std::source_location::current()) const;
  void error(std::string_view message,
             std::source_location location = std::source_location::current()) const;
  void fatal(std::string_view message,
             std::source_location location = std::source_location::current()) const;

 private:
  std::string log_path_;
  mutable std::mutex mutex_;
};

}  // namespace cvp
