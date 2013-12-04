#ifndef _ROUTING_H
#define _ROUTING_H 1

/** @file
 * Routing utilities
 **/

#include "request.h"
#include "response.h"
#include "dazibao.h"
#include "http.h"

/** maximum number of routes of the server */
#define MAX_ROUTES 16
/** default route to use when a client request the root ("/") */
#define DEFAULT_ROOT_ROUTE "/index.html"
/** directory used to search for static files */
#define PUBLIC_DIR "public_html"

/**
 * Type of a route handler. This a pointer to a function which returns an int
 * (a status, 0 if everything is ok, or an error code if there was an error. It
 * takes the following arguments:
 *  - dz (dz_t): the currently open dazibao
 *  - req (struct http_request): the request
 *  - resp (struct http_response*): the response to send back
 **/
typedef int (*route_handler)(dz_t, struct http_request, struct http_response*);

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
 * request. The response will be sent on the client socket 'sock'.
 * The currently open dazibao is given in the second argument ('dz').
 * Return 0 or -1.
 **/
int route_request(int sock, dz_t dz, struct http_request *req);

/**
 * Free the routes table and return 0.
 **/
int destroy_routes(void);

/**
 * Search for a file in PUBLIC_DIR which matches the given path. If so, serve
 * the file through 'sock'. The function returns 0 on success, -1 on failure.
 **/
int file_response(int sock, struct http_request *req);

/**
 * Send an HTTP response 'resp' on the socket 'sock', and free all memory for
 * the 'resp' struct. Returns 0 on success, -1 on error.
 **/
int http_response(int sock, struct http_response *resp);

/**
 * Same as http_response, but let you choose if you want to free the 'resp'
 * struct with 'free_resp'.
 **/
int http_response2(int sock, struct http_response *resp, char free_resp);

/**
 * Send an HTTP error status ('status', as defined in http.h) in a socket
 * ('sock'). Returns 0 on success, -1 on error. If the error status is not
 * defined in http.h, a 400 (bad request) status is sent.
 **/
int error_response(int sock, int status);

#endif
