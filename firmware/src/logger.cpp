// firmware/src/logger.cpp
#include "logger.h"
#include <stdarg.h>

LogLevel Logger::minLevel = LOG_INFO;

void Logger::log(LogLevel level, const char* tag, const char* format, ...) {
    if (level < minLevel) return;

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.printf("[%s][%s] %s\n", levelToString(level), tag, buffer);
}

void Logger::error(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_ERROR, tag, format, args);
    va_end(args);
}

void Logger::warn(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_WARN, tag, format, args);
    va_end(args);
}

void Logger::info(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_INFO, tag, format, args);
    va_end(args);
}

void Logger::debug(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_DEBUG, tag, format, args);
    va_end(args);
}

void Logger::setLogLevel(LogLevel level) {
    minLevel = level;
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LOG_ERROR: return "ERROR";
        case LOG_WARN: return "WARN";
        case LOG_INFO: return "INFO";
        case LOG_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}
