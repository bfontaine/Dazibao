#ifndef _RESPONSE_H
#define _RESPONSE_H 1

#include "http.h"

struct http_response {
        int status;
        struct http_headers *headers; /* HTTP headers */
        char **body;
        int body_len;
};

#endif
