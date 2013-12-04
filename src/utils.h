#ifndef _UTILS_H
#define _UTILS_H 1

#include <stdlib.h>

/** @file
 * Set of utilities
 **/

/** get the minimum of two values */
#define MIN(a,b) ((a)<(b)?(a):(b))
/** get the maximum of two values */
#define MAX(a,b) ((a)>(b)?(a):(b))

/** test if a value is in a range [a,b] */
#define IN_RANGE(n,a,b) ((a)<=(n) && (n)<=(b))

/** use 'perror' and exit */
#define PANIC(str) {                        \
                PERROR((str));              \
                exit(EXIT_FAILURE);         \
        }

/** print the current file and line, and use 'perror' */
#define PERROR(str) {							\
		fprintf(stderr, "ERROR - %s - l.%d - ", __FILE__, __LINE__); \
		perror((str));						\
	}

/** use 'perror' and return a value */
#define ERROR(str, i) {							\
		PERROR((str));						\
		return (i);						\
        }

/** Close a file descriptor, use 'perror' and return a value */
#define CLOSE_AND_ERROR(fd, msg, i) {       \
                if(close((fd)) == -1) {     \
                        PANIC("close");     \
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

/** @see SAVE_OFFSET */
#define RESTORE_OFFSET(d)                                 \
                if (lseek((d), __s, SEEK_SET) < 0) {   \
                        perror("[restore offset] lseek"); \
                }                                         \

/** Return the current offset in a file */
#define GET_OFFSET(fd) (lseek(fd, 0, SEEK_CUR))

/** Change the current offset in a file */
#define SET_OFFSET(fd,o) (lseek((fd),(o),SEEK_SET))

#ifdef DEBUG
/** wrapper around printf */
#define PDEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
/** 0 */
#define PDEBUG(fmt, ...) 0
#endif

/** free a pointer and set it to NULL */
#define NFREE(p) { free(p);(p) = NULL; }

/**
 * Wrapper around realloc(3) which frees the original pointer if the request
 * fails. See #35.
 **/
void *safe_realloc(void *ptr, size_t size);


/* = Logging = */

/** this is set in webutils.c but can be changed in another file. */
extern int _log_level;

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
        printf("[%5s][%-17s:%20s:%03d] " fmt "\n", \
                        s, __FILE__, __func__, __LINE__, ##__VA_ARGS__); }}

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
