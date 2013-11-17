#ifndef _ROUTING_H
#define _ROUTING_H 1

#include "dazibao.h"
#include "http.h"

#define MAX_ROUTES 16

/**
 * Type of a route handler. This a pointer to a function which returns an int
 * (a status, 0 if everything is ok, or an error code if there was an error. It
 * takes the following arguments:
 *  - dz (dz_t): the currently open dazibao
 *  - method (int): the method used by the request, as defined in http.h
 *  - path (char*): the path of the request (e.g. /index.html)
 *  - body (char*): the raw body of the request
 *  - len (int): the length of this body
 *  - status (int*): this is a result variable. It should be filled by the
 *    function to represent the status of the response
 *  - resp (char**): this is a result variable. It should be filled with a
 *    pointer to the actual response body
 *  - resplen (int*): this is a result variable. It should be filled with the
 *    length of the response body
 **/
typedef int (*route_handler)(dz_t, int, char*, char*, int, int*, char**, int*);

/**
 * Add a new route handler. 'mth' is the method used by the request.
 * 'path_suffix' is a string which will be used to match an URL path. It MUST
 * start with a slash ('/'). 'route' is a pointer on a route handler. The
 * function returns 0 on success, and -1
 * on error.
 **/
int add_route(char mth, char *path_suffix, route_handler route);

/**
 * Return a route handler for a method and a path.
 **/
route_handler get_route_handler(char mth, char *path);

/**
 * Route a request, i.e. find the matching route and accordingly respond to the
 * request. The response will be sent on the client socket 'sock', and the
 * request was made with method 'mth', on path 'path', with body 'body' of
 * length 'bodylen'. The currently open dazibao is given in the second argument
 * ('dz').
 * Return 0 or -1.
 **/
int route_request(int sock, dz_t dz, int mth, char *path, char *body,
                        int bodylen);

/**
 * Free the routes table and return 0.
 **/
int destroy_routes(void);

/**
 * Send an HTTP response on the socket 'sock', with the HTTP status 'status',
 * as defined in http.h, with the additional headers 'hs' (may be NULL), and
 * the body 'body' (may be NULL) of length 'bodylen'. Returns 0 on success, -1
 * on error.
 **/
int http_response(int sock, int status, struct http_headers *hs, char *body,
                        int bodylen);

/**
 * Send an HTTP error status ('status', as defined in http.h) in a socket
 * ('sock'). Returns 0 on success, -1 on error. If the error status is not
 * defined in http.h, a 400 (bad request) status is sent.
 **/
int error_response(int sock, int status);

#endif
