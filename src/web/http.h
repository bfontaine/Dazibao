#ifndef _HTTP_H
#define _HTTP_H 1

#define CR 13
#define LF 10

#define HTTP_VERSION "1.0"

/* Methods */
#define HTTP_M_GET 1
#define HTTP_M_POST 2
#define HTTP_M_HEAD 4
#define HTTP_M_UNSUPPORTED 8

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
#define HTTP_S_NOTALLOWED    405
#define HTTP_S_CONFLICT      409
#define HTTP_S_LENGTHREQD    411
#define HTTP_S_URITOOLONG    414

/* 5XX (server errors) */
#define HTTP_S_ERR           500
#define HTTP_S_NOTIMPL       501
#define HTTP_UNSUPP_VER      505

/* Headers internal codes
 *
 * headers will be stored in an array, so we're using small codes to access
 * each header. */
#define HTTP_H_CONTENT_TYPE   0
#define HTTP_H_CONTENT_LENGTH 1

#define HTTP_H_HOST           2
#define HTTP_H_UA             3

#define HTTP_H_ALLOW          4
#define HTTP_H_DATE           5
#define HTTP_H_SERVER         6
#define HTTP_H_POWEREDBY      7
#define HTTP_H_ACCEPT         8

/* arbitrary extentions/limits */
#define HTTP_MAX_HEADERS 16
#define HTTP_MAX_HEADER_NAME_LENGTH 64
#define HTTP_MAX_HEADER_VALUE_LENGTH 512
#define HTTP_MAX_PATH 512
#define HTTP_MAX_MTH_LENGTH 16

/* defined for easier formating strings */
#define HTTP_MAX_MTH_LENGTH_S "16"
#define HTTP_MAX_PATH_S "512"

/* Headers */
struct http_headers {
        char **headers;
};

/**
 * Test if the c-th and (c+1)th characters of a string represent an HTTP end of
 * line (CRLF). Returns 1 or 0.
 **/
char is_crlf(char *s, int c, int len);

/**
 * Get the code for an HTTP header. Return a negative value on error.
 **/
int get_http_header_code(char *str);

/**
 * Get the HTTP header for a code. Return NULL on error.
 **/
char *get_http_header_str(int code);

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
int http_add_header(struct http_headers *hs, int code, const char *value,
            char overr);

/**
 * Helper for http_headers_size.
 **/
int http_header_size(int code, char *value);

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
int destroy_http_headers(struct http_headers *hs);

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
