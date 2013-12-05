#ifndef _UTILS_H
#define _UTILS_H 1

#include <stdlib.h>
#include <unistd.h>
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

/* = errors =
 * see #6     */

/** code for write errors */
#define DZ_WRITE_ERROR         -1
/** code for read errors */
#define DZ_READ_ERROR          -2
/** code for rights errors */
#define DZ_RIGHTS_ERROR        -4
/** code for null pointers errors */
#define DZ_NULL_POINTER_ERROR  -8
/** code for memory errors */
#define DZ_MEMORY_ERROR       -16

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
 * @param the dazibao (not a pointer)
 **/
#define SAVE_OFFSET(d)                                          \
                off_t __s;                                      \
                __s = lseek((d), 0, SEEK_CUR);               \
                if (__s < 0) { perror("[save offset] lseek"); } \

/**
 * @param the dazibao (not a pointer)
 * @see SAVE_OFFSET
 */
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
 * @param ptr the pointer to realloc
 * @param size the new size
 * @return the new pointer address
 **/
void *safe_realloc(void *ptr, size_t size);

/**
 * Wrapper around write(2) to write the whole buffer instead of (sometimes)
 * only a part of it.
 * @param fd the file descriptor
 * @param buff the buffer to write from
 * @param len the length of the written data
 **/
int write_all(int fd, char *buff, int len);

/**
 * Return the extension of a file.
 * @param filename
 * @return a pointer to the part of the filename which represents the
 *         extension, or NULL if there's none. If you want to do some things
 *         with the extension while being able to free the original pointer,
 *         call strdup on it to be sure to have a new pointer.
 **/
const char *get_ext(const char *path);

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

/** type of a command-line argument */
enum arg_type {
	ARG_TYPE_INT,
	ARG_TYPE_STRING
};

/** command-line option */
struct s_option {
        /** name */
	char *name;
        /** type of the option's value */
	enum arg_type type;
        /** value of the option */
	void *value;
};

/** command-line arguments */
struct s_args {
        /** arguments count */
	int *argc;
        /** arguments */
	char ***argv;
        /** options */
	struct s_option *options;
};

/**
 * Julien's parse_args
 * @param argc arguments count
 * @param argv arguments
 * @param res the structure which will be filled
 * @param nb_opt
 **/
int jparse_args(int argc, char **argv, struct s_args *res, int nb_opt);


#endif
