// firmware/src/logger.h
#pragma once

#include <Arduino.h>

enum LogLevel {
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

class Logger {
public:
    static void log(LogLevel level, const char* tag, const char* format, ...);
    static void error(const char* tag, const char* format, ...);
    static void warn(const char* tag, const char* format, ...);
    static void info(const char* tag, const char* format, ...);
    static void debug(const char* tag, const char* format, ...);

    static void setLogLevel(LogLevel level);

private:
    static LogLevel minLevel;
    static const char* levelToString(LogLevel level);
};
