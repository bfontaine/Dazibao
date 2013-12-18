#ifndef _REQUEST_H
#define _REQUEST_H 1

/** @file
 * structures and functions to work with HTTP requests
 **/

#include "http.h"

/** A struct describing an HTTP request */
struct http_request {
        /** the request method, as defined in http.h */
        int method;
        /** requested path, e.g. "/foo" */
        char *path;
        /** supported headers */
        struct http_headers *headers;
        /** optional request body */
        char *body;
        /** length of the body (-1 if there isn't one) */
        int body_len;
};

/**
 * Create a new request: allocate enough memory and initialize all fields.
 * @return a pointer on the request
 **/
struct http_request *create_http_request(void);

/**
 * Free all the fields of a struct http_request.
 * @param req a pointer on the request
 * @return 0 on success
 **/
int destroy_http_request(struct http_request *req);

/**
 * Reset a request struct.
 * @param req a pointer on the request
 * @return 0 on success
 **/
int reset_http_request(struct http_request *req);

/**
 * Parse an HTTP request read from the socket 'sock', and fill the given
 * struct.
 * @param sock the socket to use
 * @param req
 * @return 0 on success or the appropriate HTTP status on error.
 **/
int parse_request(int sock, struct http_request *req);

/**
 * This function is a wrapper around recv to read the beginning of the input on
 * a socket line-by-line. It takes the file descriptor of a socket and return a
 * NULL-terminated string representing the next line in the input. On the end
 * of the input, the rest of the buffer is returned and eoh is set to its
 * length.  If an error occured, the function returns NULL and sets errno. It's
 * used to read the headers of an HTTP request.  The returned string is
 * dynamically allocated, so you'll need to free it later.
 * @param sock
 * @param eoh
 **/
char *next_header(int sock, int *eoh);

/**
 * Parse an header and add it to the HTTP headers struct.
 * @param line
 * @param hs
 * @return 0 on success
 **/
int parse_header(char *line, struct http_headers *hs);

/**
 * Return the boundary used by a multipart/form-data request, or NULL if
 * there's no one. Notice that it's a pointer on the Content-Type value string,
 * so if you plan to modify it you should strdup it.
 * @param req a pointer on the request
 * @return the boundary, or NULL if there's no one
 **/
char *get_request_boundary(struct http_request *req);

/** handy shortcut to get a particular header in a request */
#define REQ_HEADER(req, h) ((req).headers->headers[h])

#endif
