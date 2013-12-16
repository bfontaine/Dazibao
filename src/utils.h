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
/** code for wrong headers in dazibaos/tlvs */
#define DZ_WRONG_HEADER_ERROR -32
/** code for wrong offsets */
#define DZ_OFFSET_ERROR       -64
/** code for arguments errors */
#define DZ_ARGS_ERROR        -128
/** code for TLV types errors */
#define DZ_TLV_TYPE_ERROR    -256
/** code for mmap-related errors */
#define DZ_MMAP_ERROR        -512

/**
 * use 'perror' and exit with EXIT_FAILURE
 * @param str the string to use with perror
 * @see PERROR
 * @see ERROR
 **/
#define PANIC(str) {                        \
                PERROR((str));              \
                exit(EXIT_FAILURE);         \
        }

/**
 * print the current file and line, and use 'perror'
 * @param str the string to use with perror
 * @see PANIC
 **/
#define PERROR(str) {                                                        \
                fprintf(stderr, "ERROR - %s - l.%d - ", __FILE__, __LINE__); \
                perror((str));                                               \
        }

/**
 * print the current file and line, use 'perror' and return a value
 * @param str the string to use with perror
 * @param i the value to return
 * @see PERROR
 **/
#define ERROR(str, i) {        \
                PERROR((str)); \
                return (i);    \
        }

/**
 * SAVE_OFFSET/1 and RESTORE_OFFSET/1 macros can be used together to save
 * the current offset of a dazibao and restore it later. They MUST occur
 * in the same function, and SAVE_OFFSET MUST be called at the very beginning
 * of the function (i.e. right after the variables declarations). They use
 * a '__s' variable to save the offset, so please don't use this variable
 * in your function (this is very unlikely).
 * @param d the dazibao (not a pointer)
 **/
#define SAVE_OFFSET(d) \
                off_t __s = (d).offset

/**
 * @param d the dazibao (not a pointer)
 * @see SAVE_OFFSET
 */
#define RESTORE_OFFSET(d) \
                (d).offset = __s

/**
 * Return the current offset in a file
 * @param fd file descriptor
 * @see SET_OFFSET
 **/
#define GET_OFFSET(d) ((d).offset)

/**
 * Change the current offset in a file
 * @param fd file descriptor
 * @param o new offset
 * @see GET_OFFSET
 **/
#define SET_OFFSET(d,o) ((d).offset=o)

/**
 * wrapper to save and restore the current offset in a dazibao after a piece
 * of code
 * @param d the dazibao
 * @param code the code to wrap
 **/
#define PRESERVE_OFFSET(d,code) {SAVE_OFFSET(d);{code};RESTORE_OFFSET(d);}

/**
 * free a pointer and set it to NULL
 * @param p the pointer
 **/
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
 * Wrapper around 'strtol' which tries to parse a string as a positive decimal
 * number.
 * @param s a string
 * @return -1 on error, the parsed number on success
 **/
long str2dec_positive(char *s);

/**
 * Return the extension of a file.
 * @param path path of the file (filename)
 * @return a pointer to the part of the filename which represents the
 *         extension, or NULL if there's none. If you want to do some things
 *         with the extension while being able to free the original pointer,
 *         call strdup on it to be sure to have a new pointer.
 **/
const char *get_ext(const char *path);

/* = other utilities = */

/** helper for STR macro */
#define _STR(x) #x

/**
 * make a string of a define'd litteral number
 * @param x the define'd litteral number
 **/
#define STR(x) _STR(x)

/* == Images == */

/** this struct is used to store an TLV image's info. */
struct img_info {
        /** width */
        int width;
        /** height */
        int height;
};

/* == CLI == */

/** type of a command-line argument */
enum arg_type {
        ARG_TYPE_INT,
        ARG_TYPE_FLAG,
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
