#ifndef _UTILS_H
#define _UTILS_H 1

#include <stdlib.h>

#define MIN(a,b) ((a)<(b)?(a):(b))

#define PANIC(str) {                        \
                perror((str));              \
                exit(EXIT_FAILURE);         \
        }

#define ERROR(str, i) {                     \
                perror((str));              \
                return (i);                 \
        }

#define CLOSE_AND_ERROR(fd, msg, i) {       \
                if(close((fd)) == -1) {     \
                        PANIC("close:");    \
                }                           \
                ERROR((msg), (i));          \
        }



#endif
