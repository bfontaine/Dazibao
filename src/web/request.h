#ifndef _REQUEST_H
#define _REQUEST_H 1

#include "http.h"

#ifndef BUFFLEN
#define BUFFLEN 512
#endif

/* A struct describing an HTTP request */
/* TODO: return this when parsing a request (see also #52) */
struct http_request {
        int method;    /* Method, as defined in http.h: GET, POST */
        char *path;    /* Requested path, e.g. "/foo" */
        struct http_headers *headers; /* Supported headers */
        char *body;    /* Request body (may be NULL) */
        int body_len;  /* Length of the request body (-1 if no body) */
};

/**
 * This function is a wrapper around recv to read the beginning of the input on
 * a socket line-by-line. It takes the file descriptor of a socket and return a
 * NULL-terminated string representing the next line in the input. On the end
 * of the input, the rest of the buffer is returned and eoh is set to its
 * length.  If an error occured, the function returns NULL and sets errno. It's
 * used to read the headers of an HTTP request.  The returned string is
 * dynamically allocated, so you'll need to free it later.
 **/
char *next_header(int sock, int *eoh);

/**
 * Parse an header and add it to the HTTP headers struct. Return 0 on success.
 **/
int parse_header(char *line, struct http_headers *hs);

/**
 * Parse an HTTP request read from the socket 'sock', and fill the given
 * struct.
 *
 * The function returns 0 on success or the appropriate HTTP status on error.
 **/
int parse_request(int sock, struct http_request *req);

/**
 * Free all the fields of a struct http_request.
 **/
int destroy_http_request(struct http_request *req);

/**
 * Reset a request struct.
 **/
int reset_http_request(struct http_request *req);

#endif
