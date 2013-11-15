#ifndef _HTTP_H
#define _HTTP_H 1

#define HTTP_HEADER_CL "Content-Length:"
#define HTTP_HEADER_CL_LEN 14

/* Methods */
#define HTTP_M_GET 1
#define HTTP_M_POST 2
#define HTTP_M_UNSUPPORTED 4

/* Statuses
 *  Not all statuses are included here, only those we might need to use.
 */

struct http_statut {
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

/* Headers */
struct http_header {
        char *name;
        char *value;
};
struct http_headers {
        struct http_header *headers;
        int size;
};

/**
 * Return the phrase associated with an HTTP code. If this code is unsupported,
 * the phrase for 400 (bad request) is returned and *code is set to 400.
 **/
const char *get_http_status_phrase(int *code);

/* Headers */
struct http_header {
        char *name;
        char *value;
};
struct http_headers {
        struct http_header *headers;
        int size;
};

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
