#ifndef LOGGING_H
#define LOGGING_H

void set_log_level(int level);
void set_log_file(const char* filename);

#define LOGLEVEL_TRACE     0
#define LOGLEVEL_DEBUG    10
#define LOGLEVEL_INFO     20
#define LOGLEVEL_WARNING  30
#define LOGLEVEL_ERROR    40
#define LOGLEVEL_CRITICAL 50

#define TRACE(...)    log_message(LOGLEVEL_TRACE,    "TRACE",    __VA_ARGS__)
#define DEBUG(...)    log_message(LOGLEVEL_DEBUG,    "DEBUG",    __VA_ARGS__)
#define INFO(...)     log_message(LOGLEVEL_INFO,     "INFO",     __VA_ARGS__)
#define WARNING(...)  log_message(LOGLEVEL_WARNING,  "WARNING",  __VA_ARGS__)
#define ERROR(...)    log_message(LOGLEVEL_ERROR,    "ERROR",    __VA_ARGS__)
#define CRITICAL(...) log_message(LOGLEVEL_CRITICAL, "CRITICAL", __VA_ARGS__)

void log_message(int level, const char* severity, const char* format, ...);

#endif
