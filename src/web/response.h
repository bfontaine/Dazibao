#ifndef _RESPONSE_H
#define _RESPONSE_H 1

#include "http.h"

/* Note: a lot of these functions are copied from request.h, because HTTP
 * response and HTTP requests are highly similar.
 */

/**
 * An HTTP response
 **/
struct http_response {
        /** status of the response */
        int status;
        /** headers of the response */
        struct http_headers *headers;
        /** optional body */
        char **body;
        /** length of the body, -1 if there's no one */
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
