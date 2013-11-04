#ifndef _UTILS_H
#define _UTILS_H 1

#include <stdlib.h>

#define MIN(a,b) ((a)<(b)?(a):(b))

#define PANIC(str) {                        \
                perror((str));              \
                exit(EXIT_FAILURE);         \
        }

#define ERROR(str, i) {							\
		fprintf(stderr, "ERROR - %s - l.%d - ", __FILE__, __LINE__); \
                perror((str));						\
                return (i);						\
        }

#define CLOSE_AND_ERROR(fd, msg, i) {       \
                if(close((fd)) == -1) {     \
                        PANIC("close");    \
                }                           \
                ERROR((msg), (i));          \
        }

#define GET_OFFSET(fd) (lseek(fd, 0, SEEK_CUR))

#define SET_OFFSET(fd,o) (lseek((fd),(o),SEEK_SET))

#ifdef DEBUG
#define PDEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PDEBUG(fmt, ...) 0
#endif

#endif
