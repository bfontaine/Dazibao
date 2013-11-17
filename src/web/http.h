#ifndef _HTTP_H
#define _HTTP_H 1

#include "utils.h"

#define HTTP_HEADER_CL "Content-Length:"
#define HTTP_HEADER_CL_LEN 14

/* Methods */
#define HTTP_M_GET 1
#define HTTP_M_POST 2
#define HTTP_M_UNSUPPORTED 4

#define HTTP_M_ANY (HTTP_M_GET|HTTP_M_POST)

/* Statuses
 *  Not all statuses are included here, only those we might need to use.
 */

struct http_status {
        int code;
        char *phrase;
};

/* 2XX (success) */
#define HTTP_S_OK            200
#define HTTP_S_CREATED       201
#define HTTP_S_NO_CT         204
#define HTTP_S_RESET_CT      205

/* 3XX (redirections) */
#define HTTP_S_MLTPL_CHOICES 300
#define HTTP_S_MOVED         301
#define HTTP_S_FOUND         302

/* 4XX (client errors) */
#define HTTP_S_BADREQ        400
#define HTTP_S_FORBIDDEN     403
#define HTTP_S_NOTFOUND      404
#define HTTP_S_CONFLICT      409
#define HTTP_S_LENGTHREQD    411
#define HTTP_S_URITOOLONG    414

/* 5XX (server errors) */
#define HTTP_S_ERR           500
#define HTTP_S_NOTIMPL       501
#define HTTP_UNSUPP_VER      505

/* custom extentions/limits */
#define HTTP_MAX_PATH 512
#define HTTP_MAX_MTH_LENGTH 16
#define HTTP_MAX_HEADERS 16

/* Headers */
struct http_header {
        char *name;
        char *value;
};
struct http_headers {
        struct http_header **headers;
        int size;
};

/**
 * Initialize a struct http_headers. Return 0 or -1.
 **/
int http_init_headers(struct http_headers *hs);

/**
 * Add an header to a struct http_headers. If 'overr' is set to '0', the
 * function will not attempt to override an existing header and will instead
 * return -2 if it already exists. If 'overr' is set to another value, the
 * function will override an existing header if there's one. It returns 0 on
 * success, -1 if there was an error.
 **/
int http_add_header(struct http_headers *hs, char *name, char *value,
                        int overr);

/**
 * Helper for http_headers_size.
 **/
int http_header_size(struct http_header *h) WARN_UNUSED;
/**
 * Return the size of the string representation of a list of headers, without
 * \0.
 **/
int http_headers_size(struct http_headers *hs);

/**
 * Return a NULL-terminated string representation of an header.
 **/
char *http_header_string(struct http_header *h);

/**
 * Return a NULL-terminated string representation of a list of headers.
 **/
char *http_headers_string(struct http_headers *hs);

/**
 * Free all the allocated memory for a list of headers. Return 0 or -1.
 **/
int http_destroy_headers(struct http_headers *hs);

/**
 * Return the phrase associated with an HTTP code. If this code is unsupported,
 * the phrase for 400 (bad request) is returned and *code is set to 400.
 **/
const char *get_http_status_phrase(int *code);

/**
 * Return the code for an HTTP method. It matches methods by their first
 * letter, since the only ones we support are GET and POST.
 **/
int http_mth(char *s);

#endif
