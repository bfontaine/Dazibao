#ifndef _WEBUTILS_H
#define _WEBUTILS_H 1

/* = Global server properties = */
struct wserver_info {
        int port;
        char *hostname;
        char *dzname;
} WSERVER;

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
        printf("[%5s][%-17s:%20s:%03d] " fmt "\n", \
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

/* = I/O = */

/**
 * Wrapper around write(2) to write the whole buffer instead of (sometimes)
 * only a part of it.
 **/
int write_all(int fd, char *buff, int len);

#endif
