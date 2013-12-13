#ifndef _LOGGING_H
#define _LOGGING_H 1

#include <stdio.h>
#include <unistd.h>
#include <time.h>

/**
 * @file
 * Logging utilities for servers
 **/

/**
 * global variable used to set the log level. This is set in logging.c but can
 * be changed in another file.
 **/
extern int _log_level;

/* set this to 0 to get each log entry on one line instead of two */
extern char _log_newline;

/** "debug" log level */
#define LOG_LVL_DEBUG  50
/** "info" log level */
#define LOG_LVL_INFO   40
/** "warn" log level */
#define LOG_LVL_WARN   30
/** "error" log level */
#define LOG_LVL_ERROR  20
/** "fatal" log level */
#define LOG_LVL_FATAL  10

/** Don't use this macro directly */
#define _LOG(lvl, s, fmt, ...) { \
        if ((lvl) <= (_log_level)) { \
        struct tm ts; \
        time_t t = time(NULL); \
        char h[16], nl = _log_newline?'\n':' '; \
        localtime_r(&t, &ts); \
        strftime(h, 16, "%T", &ts); \
        fprintf(stderr, "[%5s][%8s] %s%c %-17s:%03d] " fmt "\n", \
                        s, h, __func__, nl, __FILE__, __LINE__, \
                        ##__VA_ARGS__); }}

/* Use these instead.
 * Examples:
 *      LOGDEBUG("yo");
 *      LOGERROR("2+2=%d", 5);
 *
 * Don't put a \n at the end of the format string */

/** add an entry to a log using the "debug" level */
#define LOGDEBUG(fmt, ...) _LOG(LOG_LVL_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
/** add an entry to a log using the "info" level */
#define LOGINFO(fmt, ...)  _LOG(LOG_LVL_INFO,  "INFO", fmt, ##__VA_ARGS__)
/** add an entry to a log using the "warn" level */
#define LOGWARN(fmt, ...)  _LOG(LOG_LVL_WARN,  "WARN", fmt, ##__VA_ARGS__)
/** add an entry to a log using the "error" level */
#define LOGERROR(fmt, ...) _LOG(LOG_LVL_ERROR, "ERROR", fmt, ##__VA_ARGS__)
/** add an entry to a log using the "fatal" level */
#define LOGFATAL(fmt, ...) _LOG(LOG_LVL_FATAL, "FATAL", fmt, ##__VA_ARGS__)

#endif
