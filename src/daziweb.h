#ifndef _DAZIWEB_H
#define _DAZIWEB_H 1

#include "utils.h"

#define DEFAULT_PORT 3437
#define MAX_QUEUE 64

#ifndef BUFFLEN
#define BUFFLEN 512
#endif

#define CR 13
#define LF 10

/**
 * Test if the c-th and (c+1)th characters of a string represent an HTTP end of
 * line (CRLF). Returns 1 or 0.
 **/
char is_crlf(char *s, int c, int len);

/**
 * This function is a wrapper around recv to read the beginning of the input on
 * a socket line-by-line. It takes the file descriptor of a socket and return a
 * NULL-terminated string representing the next line in the input. On the end of
 * the input, an empty string is returned. If an error occured, the function
 * returns NULL. It's used to read the headers of an HTTP request.
 * The returned string is dynamically allocated, so you'll need to free it
 * later.
 **/
char *next_header(int sock);

#endif
