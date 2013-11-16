#ifndef _WEBUTILS_H
#define _WEBUTILS_H 1

/* = Logging = */

/* this is set in webutils.c but can be changed in another file. */
extern int _wlog_level;

#define WLOG_LVL_DEBUG  50
#define WLOG_LVL_INFO   40
#define WLOG_LVL_WARN   30
#define WLOG_LVL_ERROR  20
#define WLOG_LVL_FATAL  10

/* Don't use this macro directly */
#define _WLOG(lvl, s, fmt, ...) { \
        if ((lvl) <= (_wlog_level)) { \
        printf("[%5s][daziweb:%10s:%15s:%03d] " fmt "\n", \
                        s, __FILE__, __func__, __LINE__, ##__VA_ARGS__); }}

/* Use these instead.
 * Examples:
 *      WLOGDEBUG("yo");
 *      WLOGERROR("2+2=%d", 5);
 *
 * Don't put a \n at the end of the format string */
#define WLOGDEBUG(fmt, ...) _WLOG(WLOG_LVL_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define WLOGINFO(fmt, ...)  _WLOG(WLOG_LVL_INFO,  "INFO", fmt, ##__VA_ARGS__)
#define WLOGWARN(fmt, ...)  _WLOG(WLOG_LVL_WARN,  "WARN", fmt, ##__VA_ARGS__)
#define WLOGERROR(fmt, ...) _WLOG(WLOG_LVL_ERROR, "ERROR", fmt, ##__VA_ARGS__)
#define WLOGFATAL(fmt, ...) _WLOG(WLOG_LVL_FATAL, "FATAL", fmt, ##__VA_ARGS__)

#endif
