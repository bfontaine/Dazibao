#ifndef _REQUEST_H
#define _REQUEST_H 1

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
 * the input, the rest of the buffer is returned and eoh is set to its length.
 * If an error occured, the function returns NULL and sets errno. It's used to
 * read the headers of an HTTP request.  The returned string is dynamically
 * allocated, so you'll need to free it later.
 **/
char *next_header(int sock, int *eoh);

/**
 * **NOTE** this function is currently a draft.
 **/
void parse_request(int sock);

#endif
