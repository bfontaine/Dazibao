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

/* Headers internal codes
 *
 * headers will be stored in an array, so we're using small ones to access
 * each header. */
#define HTTP_H_CONTENT_TYPE   0
#define HTTP_H_CONTENT_LENGTH 1

#define HTTP_H_HOST           2
#define HTTP_H_UA             3

#define HTTP_H_ALLOW          4
#define HTTP_H_DATE           5
#define HTTP_H_SERVER         6
#define HTTP_H_POWEREDBY      7

/* custom extentions/limits */
#define HTTP_MAX_HEADERS 8
#define HTTP_MAX_PATH 512
#define HTTP_MAX_MTH_LENGTH 16

/* Headers */
struct http_headers {
        char **headers;
};

/**
 * Retrieve either the code of an header or its string. If 'str' points to the
 * address of a NULL pointer and 'code' points to the address of a valid header
 * code, 'str' is filled with the address of a string describing the header. On
 * the opposite, if '*str' is a valid header string and '*code' is negative,
 * it's filled with the header code. The function returns 0 on success, -1 if
 * the header doesn't exist, or -2 if there was an error.
 *
 * Be carefull to free '*str' if you use this function to get the string for an
 * header.
 **/
int http_header_code_str(char **str, int *code);

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
 *
 * Please also note that only a small set of HTTP headers are supported
 **/
int http_add_header(struct http_headers *hs, int code, char *value,
            char overr);

/**
 * Helper for http_headers_size.
 **/
int http_header_size(int code, char *value) WARN_UNUSED;

/**
 * Return the size of the string representation of a list of headers, without
 * \0.
 **/
int http_headers_size(struct http_headers *hs);

/**
 * Return a NULL-terminated string representation of an header.
 **/
char *http_header_string(int code, char *value);

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
 * Return the code for an HTTP method.
 **/
int http_mth(char *s);

#endif
