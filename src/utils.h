#ifndef _UTILS_H
#define _UTILS_H 1

#include <limits.h>
#include <stdlib.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define STRTOL_ERR(n) ((n) == LONG_MIN || (n) == LONG_MAX)

#define PANIC(str) {                        \
                PERROR((str));              \
                exit(EXIT_FAILURE);         \
        }

#define PERROR(str) {							\
		fprintf(stderr, "ERROR - %s - l.%d - ", __FILE__, __LINE__); \
		perror((str));						\
	}

#define ERROR(str, i) {							\
		PERROR((str));						\
		return (i);						\
        }

#define CLOSE_AND_ERROR(fd, msg, i) {       \
                if(close((fd)) == -1) {     \
                        PANIC("close");    \
                }                           \
                ERROR((msg), (i));          \
        }

/**
 * SAVE_OFFSET/1 and RESTORE_OFFSET/1 macros can be used together to save
 * the current offset of a dazibao and restore it later. They MUST occur
 * in the same function, and SAVE_OFFSET MUST be called at the very beginning
 * of the function (i.e. right after the variables declarations). They use
 * a '__s' variable to save the offset, so please don't use this variable
 * in your function (this is very unlikely).
 **/
#define SAVE_OFFSET(d)                                          \
                off_t __s;                                      \
                __s = lseek((d), 0, SEEK_CUR);               \
                if (__s < 0) { perror("[save offset] lseek"); } \

#define RESTORE_OFFSET(d)                                 \
                if (lseek((d), __s, SEEK_SET) < 0) {   \
                        perror("[restore offset] lseek"); \
                }                                         \

#define GET_OFFSET(fd) (lseek(fd, 0, SEEK_CUR))

#define SET_OFFSET(fd,o) (lseek((fd),(o),SEEK_SET))

#ifdef DEBUG
#define PDEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PDEBUG(fmt, ...) 0
#endif

#define NFREE(p) { free(p);(p) = NULL; }

/* see sourcefrog.net/weblog/software/languages/C/warn-unused.html */
#ifdef __GNUC__
#define WARN_UNUSED  __attribute__((warn_unused_result))
#else
#define WARN_UNUSED
#endif

/**
 * Wrapper around realloc(3) which frees the original pointer if the request
 * fails. See #35.
 **/
void *safe_realloc(void *ptr, size_t size);


/* = Logging = */

/* this is set in webutils.c but can be changed in another file. */
extern int _log_level;

#define LOG_LVL_DEBUG  50
#define LOG_LVL_INFO   40
#define LOG_LVL_WARN   30
#define LOG_LVL_ERROR  20
#define LOG_LVL_FATAL  10

/* Don't use this macro directly */
#define _LOG(lvl, s, fmt, ...) { \
        if ((lvl) <= (_log_level)) { \
        printf("[%5s][%-17s:%20s:%03d] " fmt "\n", \
                        s, __FILE__, __func__, __LINE__, ##__VA_ARGS__); }}

/* Use these instead.
 * Examples:
 *      LOGDEBUG("yo");
 *      LOGERROR("2+2=%d", 5);
 *
 * Don't put a \n at the end of the format string */
#define LOGDEBUG(fmt, ...) _LOG(LOG_LVL_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define LOGINFO(fmt, ...)  _LOG(LOG_LVL_INFO,  "INFO", fmt, ##__VA_ARGS__)
#define LOGWARN(fmt, ...)  _LOG(LOG_LVL_WARN,  "WARN", fmt, ##__VA_ARGS__)
#define LOGERROR(fmt, ...) _LOG(LOG_LVL_ERROR, "ERROR", fmt, ##__VA_ARGS__)
#define LOGFATAL(fmt, ...) _LOG(LOG_LVL_FATAL, "FATAL", fmt, ##__VA_ARGS__)



#endif
