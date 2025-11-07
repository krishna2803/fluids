#include "logger.hh"

static std::unique_ptr<Logger> s_logger_instance = nullptr;

Logger &Logger::Get() {
  if (!s_logger_instance)
    s_logger_instance = std::unique_ptr<Logger>(new Logger());
  return *s_logger_instance;
}

Logger::~Logger() {
  if (backend_started)
    quill::Backend::stop();
}

void Logger::init() {
  if (qlogger)
    return;

  if (!backend_started) {
    quill::Backend::start();
    backend_started = true;
  }

  auto console_sink =
      quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console");
  qlogger =
      quill::Frontend::create_or_get_logger("root", std::move(console_sink));
  qlogger->set_log_level(quill::LogLevel::TraceL3);
}
