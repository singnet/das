/**
 * @file Logger.h
 * @brief Logger class for logging messages with different severity levels.
 * @details This file contains macros and functions for logging messages with different severity levels
 * (DEBUG, INFO, ERROR) along with timestamps. The logging level can be configured using the LOG_LEVEL
 *  macro. The default logging level is set to DEBUG_LEVEL.
 * usage:
 * 1. Include this header file in your source code.
 * 2. Set the desired logging level by defining the LOG_LEVEL (NO_LOG, ERROR_LEVEL, INFO_LEVEL,
 * DEBUG_LEVEL) macro before including this file.
 * 3. Use the logging macros (LOG_DEBUG, LOG_INFO, LOG_ERROR) to log messages at different severity
 * levels.
 * 4. The log messages will be printed to the standard output (stdout) or standard error (stderr) with a
 * timestamp.
 * 5. The log messages will be formatted as follows:
 * (DEBUG and ERROR): [TIMESTAMP] | [LOG_LEVEL] | [FILE_NAME] | [FUNCTION_NAME] : [LINE_NUMBER] |
 * [MESSAGE]
 * (INFO): [TIMESTAMP] | [LOG_LEVEL] | [MESSAGE]
 * 6. The timestamp will be in the format YYYY-MM-DD HH:MM:SS.
 * Example:
 *  #define LOG_LEVEL DEBUG_LEVEL
 *  #include "Logger.h"
 *  int main() {
 *      LOG_DEBUG("This is a debug message. " << LOG_LEVEL << " is the log level");
 *      LOG_INFO("This is an info message and the log level is " << LOG_LEVEL);
 *      LOG_ERROR("This is an error message and the log level is " << LOG_LEVEL);
 *      return 0;
 *  }
 */
#pragma once
#include <string.h>
#include <time.h>

#include <iostream>

static inline char* timenow();

#define NO_LOG 0x00
#define ERROR_LEVEL 0x01
#define INFO_LEVEL 0x02
#define DEBUG_LEVEL 0x03

#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG_LEVEL
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(msg)                                                                            \
    (std::cout << timenow() << " | "                                                              \
               << "[DEBUG] | " << __FILE__ << " | " << __FUNCTION__ << " : " << __LINE__ << " | " \
               << msg << std::endl)
#else
#define LOG_DEBUG(msg)
#endif

#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(msg)                \
    (std::cout << timenow() << " | " \
               << "[INFO] | " << msg << std::endl)
#else
#define LOG_INFO(msg)
#endif

#if LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(msg)                                                                            \
    (std::cerr << timenow() << " | "                                                              \
               << "[ERROR] | " << __FILE__ << " | " << __FUNCTION__ << " : " << __LINE__ << " | " \
               << msg << std::endl)
#else
#define LOG_ERROR(msg)
#endif

static inline char* timenow() {
    static char buffer[64];
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);

    return buffer;
}
