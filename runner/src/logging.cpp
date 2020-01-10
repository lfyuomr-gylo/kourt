#include <mutex>

#include "logging.h"

static LoggingLevel active_level = LoggingLevel::kTrace;
static bool is_active_level_initialized = false;
static std::mutex active_level_mutex;

static LoggingLevel LoggingLevelFromString(const char *raw_env_active_level) {
  const std::string env_active_level(raw_env_active_level);
  if (env_active_level == "TRACE") {
    return LoggingLevel::kTrace;
  } else if (env_active_level == "DEBUG") {
    return LoggingLevel::kDebug;
  } else if (env_active_level == "INFO") {
    return LoggingLevel::kInfo;
  } else if (env_active_level == "WARN") {
    return LoggingLevel::kWarn;
  } else if (env_active_level == "ERROR") {
    return LoggingLevel::rError;
  } else {
    WARN("Got unexpected logging level from environment: '%s'", raw_env_active_level);
    return active_level;
  }
}

static void InitActiveLevelIfNecessary() {
  if (!is_active_level_initialized) {
    std::scoped_lock lock(active_level_mutex);
    if (!is_active_level_initialized) {
      is_active_level_initialized = true;
      const char *raw_env_active_level = getenv("KOURT_RUNNER_LOG_LEVEL");
      if (raw_env_active_level) {
        active_level = LoggingLevelFromString(raw_env_active_level);
      }
    }
  }
}

const char *LoggingLevelToString(LoggingLevel level) {
  switch (level) {
    case LoggingLevel::kTrace: return "TRACE";
    case LoggingLevel::kDebug: return "DEBUG";
    case LoggingLevel::kInfo: return "INFO";
    case LoggingLevel::kWarn: return "WARN";
    case LoggingLevel::rError: return "ERROR";
    default: throw std::logic_error("Unknown logging level: " + std::to_string(static_cast<int>(level)));
  }
}

bool IsLoggingLevelActive(LoggingLevel level) {
  InitActiveLevelIfNecessary();
  return static_cast<int>(level) >= static_cast<int>(active_level);
}
