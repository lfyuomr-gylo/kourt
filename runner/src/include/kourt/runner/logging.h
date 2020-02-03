#ifndef RUNNER_SRC_LOGGING_H_
#define RUNNER_SRC_LOGGING_H_

#include <ctime>
#include <cstdio>
#include <string>
#include <stdexcept>

enum class LoggingLevel {
  kTrace = 10,
  kDebug = 20,
  kInfo = 30,
  kWarn = 40,
  kError = 50
};

const char *LoggingLevelToString(LoggingLevel level);

bool IsLoggingLevelActive(LoggingLevel level);

template<typename... Params>
void KourtDoLog(LoggingLevel level,
                const char *file_name,
                size_t line_number,
                const std::string &message_template,
                Params... message_params) {
  timespec raw_now{};
  clock_gettime(CLOCK_REALTIME, &raw_now);
  tm now{};
  localtime_r(&raw_now.tv_sec, &now);

  const char *line_end = (message_template[message_template.length() - 1] == '\n')
      ? ""
      : "\n";
  std::string template_str = "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3u\t%s\t%s:%d\t" + message_template + line_end;
  fprintf(
      stderr,
      template_str.c_str(),
      1900 + now.tm_year,
      now.tm_mon + 1,
      now.tm_mday,
      now.tm_hour,
      now.tm_min,
      now.tm_sec,
      raw_now.tv_nsec / 1'000'000,
      LoggingLevelToString(level),
      file_name,
      line_number,
      message_params...
  );
}

#define LOG(level, args...) \
  if (IsLoggingLevelActive(level)) { \
    KourtDoLog(level, __FILE__, __LINE__, args); \
  }

#ifndef TRACE
#define TRACE(args...) LOG(LoggingLevel::kTrace, args)
#else
#error "TRACE macro is already defined"
#endif

#ifndef DEBUG
#define DEBUG(args...) LOG(LoggingLevel::kDebug, args)
#else
#error "DEBUG macro is already defined"
#endif

#ifndef INFO
#define INFO(args...) LOG(LoggingLevel::kInfo, args)
#else
#error "INFO macro is already defined"
#endif

#ifndef WARN
#define WARN(args...) LOG(LoggingLevel::kWarn, args)
#else
#error "WARN macro is already defined"
#endif

#ifndef ERROR
#define ERROR(args...) LOG(LoggingLevel::kError, args)
#else
#error "ERROR macro is already defined"
#endif

#endif //RUNNER_SRC_LOGGING_H_
