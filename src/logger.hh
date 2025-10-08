#pragma once

#include <memory>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>

class Logger {
public:
  static Logger &Get();

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(Logger &&) = delete;
  ~Logger();

  void init();
  quill::Logger *get_logger() const { return qlogger; }

private:
  Logger() = default;
  quill::Logger *qlogger = nullptr;
  bool backend_started = false;
};

#define LOG_TRACE(fmt, ...)                                                    \
  LOG_TRACE_L3(Logger::Get().get_logger(), fmt, ##__VA_ARGS__)
#define LOG_DEBUG_MSG(fmt, ...)                                                \
  LOG_DEBUG(Logger::Get().get_logger(), fmt, ##__VA_ARGS__)
#define LOG_INFO_MSG(fmt, ...)                                                 \
  LOG_INFO(Logger::Get().get_logger(), fmt, ##__VA_ARGS__)
#define LOG_WARNING_MSG(fmt, ...)                                              \
  LOG_WARNING(Logger::Get().get_logger(), fmt, ##__VA_ARGS__)
#define LOG_ERROR_MSG(fmt, ...)                                                \
  LOG_ERROR(Logger::Get().get_logger(), fmt, ##__VA_ARGS__)