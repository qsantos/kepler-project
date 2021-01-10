#include "logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static int log_level = 0;
static FILE* log_file = NULL;

void set_log_level(int level) {
    log_level = level;
}

void set_log_file(const char* filename) {
    FILE* new_log_file = fopen(filename, "w");
    if (new_log_file == NULL) {
        CRITICAL("Could not open %s\n", filename);
    }
    log_file = new_log_file;
}

void log_message(int level, const char* severity, const char* format, ...) {
    if (level < log_level) {
        return;
    }

    char buffer[4096];
    size_t offset = 0;

    // automatic prefix
    time_t now = time(NULL);
    struct tm* parts = gmtime(&now);
    int a = snprintf(
        buffer + offset,
        sizeof(buffer) - offset,
        "%04d-%02d-%02dT%02d:%02d:%02dZ  %8s  ",
        1900 + parts->tm_year,
        parts->tm_mon + 1,
        parts->tm_mday,
        parts->tm_hour,
        parts->tm_min,
        parts->tm_sec,
        severity
    );
    if (a >= 0) {
        offset += (size_t) a;
    }

    // message given as argument
    va_list args;
    va_start(args, format);
    int b = vsnprintf(buffer + offset, sizeof(buffer) - offset, format, args);
    va_end(args);
    if (b >= 0) {
        offset += (size_t) b;

        // force newline
        if (offset < sizeof(buffer) - 1 && buffer[offset - 1] != '\n') {
            buffer[offset] = '\n';
            buffer[offset + 1] = '\0';
        }
    }

    fputs(buffer, stderr);
    if (log_file != NULL) {
        fputs(buffer, log_file);
        fflush(log_file);
    }
}
