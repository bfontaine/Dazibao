#ifndef _UTILS_H
#define _UTILS_H 1

#include <stdlib.h>

#define MIN(a,b) ((a)<(b)?(a):(b))

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

#endif
