#ifndef _UTILS_H
#define _UTILS_H 1

#include <stdlib.h>
#include <unistd.h>
#include <sys/type.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

/** @file
 * Set of utilities
 **/

#ifndef BUFFLEN
/** size of buffers used in various functions */
#define BUFFLEN 512
#endif

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
 * Check if path exist file , regular file
 * and is not to large
 **/
int safe_path(const char * path, int flag_access){
/**
 * Wrapper around realloc(3) which frees the original pointer if the request
 * fails. See #35.
 **/
void *safe_realloc(void *ptr, size_t size);

/**
 * Wrapper around write(2) to write the whole buffer instead of (sometimes)
 * only a part of it.
 **/
int write_all(int fd, char *buff, int len);


/* = Logging = */
#include "logging.h"

/* = other utilities = */

/** helper for STR macro */
#define _STR(x) #x

/**
 * make a string of a #define'd litteral number
 * @param x the #define'd litteral number
 **/
#define STR(x) _STR(x)


enum arg_type {
	ARG_TYPE_INT,
	ARG_TYPE_STRING
};

struct s_option {
	char *name;
	enum arg_type type;
	void *value;
};

struct s_args {
	int *argc;
	char ***argv;
	struct s_option *options;
};


int jparse_args(int argc, char **argv, struct s_args *res, int nb_opt);


#endif
