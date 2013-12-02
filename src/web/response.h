#ifndef _RESPONSE_H
#define _RESPONSE_H 1

#include "http.h"

/* Note: a lot of these functions are copied from request.h, because HTTP
 * response and HTTP requests are highly similar.
 */

struct http_response {
        int status;
        struct http_headers *headers; /* HTTP headers */
        char **body;
        int body_len;
};

/**
 * Create a new HTTP response.
 **/
struct http_response *create_http_response(void);

/**
 * Free all the memory allocated for an HTTP response.
 **/
int destroy_http_response(struct http_response *resp);

#endif
