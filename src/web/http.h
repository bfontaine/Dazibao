#ifndef _HTTP_H
#define _HTTP_H 1

#define HTTP_HEADER_CL "Content-Length:"
#define HTTP_HEADER_CL_LEN 14

/* Methods */
#define HTTP_M_GET 1
#define HTTP_M_POST 2
#define HTTP_M_UNSUPPORTED 4

/* Statuses */
#define HTTP_S_OK  200
#define HTTP_S_ERR 500

#define HTTP_S_BADREQ 400

/* custom extentions/limits */
#define HTTP_MAX_PATH 512
#define HTTP_MAX_MTH_LENGTH 16

/**
 * Return the code for an HTTP method. It matches methods by their first
 * letter, since the only ones we support are GET and POST.
 **/
int http_mth(char *s);

#endif
