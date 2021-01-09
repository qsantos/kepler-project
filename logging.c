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

    time_t now = time(NULL);
    char timestamp[21];
    strftime(timestamp, sizeof(timestamp), "%FT%TZ", gmtime(&now));

    const char* message = NULL;
    char buffer[4096];
    va_list args;
    int a = snprintf(buffer, sizeof(buffer), "%20s  %8s  ", timestamp, severity);
    if (a < 0) {
        message = "Failed to format message prefix\n";
        goto output_message;
    }

    va_start(args, format);
    int b = vsnprintf(buffer + a, sizeof(buffer) - (size_t) a, format, args);
    va_end(args);
    if (b < 0) {
        message = "Failed to format message\n";
        goto output_message;
    }

    size_t n = (size_t) (a + b);
    if (n < sizeof(buffer) - 1 && buffer[n - 1] != '\n') {
        buffer[n] = '\n';
        buffer[n + 1] = '\0';
    }
    message = buffer;

output_message:
    fputs(message, stderr);
    if (log_file != NULL) {
        fputs(message, log_file);
        fflush(log_file);
    }
}
