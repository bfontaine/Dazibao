#ifndef _HTTP_H
#define _HTTP_H 1

/** @file
 * Utilities to work with the HTTP protocol
 **/

/** a "CR" character (carriage return) */
#define CR 13

/** a "LF" character (linefeed) */
#define LF 10

/** version of HTTP used by the server */
#define HTTP_VERSION "1.1"

/* Methods */
/** code for a GET method */
#define HTTP_M_GET 1
/** code for a POST method */
#define HTTP_M_POST 2
/** code for a HEAD method */
#define HTTP_M_HEAD 4
/** code for an unsupported method */
#define HTTP_M_UNSUPPORTED 8

/**
 * code for GET and POST methods. This is used when a route is registered for
 * both GET and POST methods
 **/
#define HTTP_M_ANY (HTTP_M_GET|HTTP_M_POST)

/* Statuses
 *  Not all statuses are included here, only those we might need to use.
 */

/**
 * An HTTP status
 **/
struct http_status {
        /** The code of the status, e.g.: 200 **/
        int code;

        /** The phrase of the status, used in server responses, e.g.: "OK" **/
        char *phrase;
};

/* 2XX (success) */
/** "OK" HTTP status */
#define HTTP_S_OK            200
/** 201 HTTP status */
#define HTTP_S_CREATED       201
/** 204 HTTP status */
#define HTTP_S_NO_CT         204
/** 205 HTTP status */
#define HTTP_S_RESET_CT      205

/* 3XX (redirections) */
/** 300 HTTP status */
#define HTTP_S_MLTPL_CHOICES 300
/** 301 HTTP status */
#define HTTP_S_MOVED         301
/** 302 HTTP status */
#define HTTP_S_FOUND         302

/* 4XX (client errors) */
/** 400 HTTP status */
#define HTTP_S_BADREQ        400
/** 403 HTTP status */
#define HTTP_S_FORBIDDEN     403
/** 404 HTTP status */
#define HTTP_S_NOTFOUND      404
/** 405 HTTP status */
#define HTTP_S_NOTALLOWED    405
/** 409 HTTP status */
#define HTTP_S_CONFLICT      409
/** 411 HTTP status */
#define HTTP_S_LENGTHREQD    411
/** 415 HTTP status */
#define HTTP_S_URITOOLONG    414

/* 5XX (server errors) */
/** generic server error HTTP status */
#define HTTP_S_ERR           500
/** "Not Implemented" HTTP status */
#define HTTP_S_NOTIMPL       501
/** "Unsupported Version" HTTP status */
#define HTTP_UNSUPP_VER      505

/* Headers internal codes
 *
 * headers will be stored in an array, so we're using small codes to access
 * each header. */
/** code for a "Content-Type" HTTP header */
#define HTTP_H_CONTENT_TYPE   0
/** code for a "Content-Length" HTTP header */
#define HTTP_H_CONTENT_LENGTH 1

/** code for an "Host" HTTP header */
#define HTTP_H_HOST           2
/** code for an "User-Agent" HTTP header */
#define HTTP_H_UA             3

/** code for an "Allow" HTTP header */
#define HTTP_H_ALLOW          4
/** code for a "Date" HTTP header */
#define HTTP_H_DATE           5
/** code for a "Server" HTTP header */
#define HTTP_H_SERVER         6
/** code for an "X-Powered-By" HTTP header */
#define HTTP_H_POWEREDBY      7
/** code for an "Accept" HTTP header */
#define HTTP_H_ACCEPT         8
/** code for an "If-Modified-Since" HTTP header */
#define HTTP_H_IFMODIFSINCE   9
/** code for a "Last Modified" HTTP header */
#define HTTP_H_LASTMODIF     10

/* arbitrary extentions/limits */
/** maximum number of HTTP headers in a request/response */
#define HTTP_MAX_HEADERS 16
/** maximum size of an HTTP header name field */
#define HTTP_MAX_HEADER_NAME_LENGTH 64
/** maximum size of an HTTP header value field */
#define HTTP_MAX_HEADER_VALUE_LENGTH 512
/** maximum path in an HTTP request */
#define HTTP_MAX_PATH 512
/** maximum length of a method in an HTTP request */
#define HTTP_MAX_MTH_LENGTH 16

/* defined for easier strings formating - TODO use a cleaner alternative */
/** Same as HTTP_MAX_MTH_LENGTH but as a string **/
#define HTTP_MAX_MTH_LENGTH_S "16"
/** Same as HTTP_MAX_PATH but as a string **/
#define HTTP_MAX_PATH_S "512"

/** Format used for HTTP dates */
#define HTTP_DATE_FMT "%a, %d %b %Y %T GMT"

/** A list of HTTP headers */
struct http_headers {
        /** the list of headers */
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
