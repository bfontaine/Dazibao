#ifndef _ROUTING_H
#define _ROUTING_H 1

/** @file
 * Routing utilities
 **/

#include "request.h"
#include "response.h"
#include "mdazibao.h"
#include "http.h"

/** maximum number of routes of the server */
#define MAX_ROUTES 16
/** default route to use when a client request the root ("/") */
#define DEFAULT_ROOT_ROUTE "/index.html"
/** directory used to search for static files */
#define PUBLIC_DIR "public_html"

/**
 * Type of a route handler. This a pointer to a function which returns an int
 * (0 if everything is ok, an HTTP status or an error code if there was an
 * error. It takes the following arguments:
 *  - dz (dz_t*): a pointer to the currently open dazibao
 *  - req (struct http_request): the request
 *  - resp (struct http_response*): the response to send back
 **/
typedef int (*route_handler)(dz_t*, struct http_request,
                struct http_response*);

/**
 * Add a new route handler.
 * @param mth method used by the request
 * @param path_prefix a string which will be used to match an URL path. It MUST
 *        start with a slash ('/').
 * @param route a pointer on a route handler
 * @return 0 on success, and -1 on error
 **/
int add_route(char mth, char *path_prefix, route_handler route);

/**
 * code returned by get_route_handler when the request method is wrong.
 * @see get_route_handler
 **/
#define ROUTING_WRONG_MTH 1
/**
 * code returned by get_route_handler when the requested path is wrong.
 * @see get_route_handler
 **/
#define ROUTING_WRONG_PATH 2

/**
 * Return a route handler for a method and a path.
 * @param mth the method
 * @param path the path
 * @param status if not NULL and the route handler cannot be found, it'll be
 * filled with either ROUTING_WRONG_MTH (good path but wrong method) or
 * ROUTING_WRONG_PATH (wrong path).
 * @return the route handler if there's one, or NULL
 * @see ROUTING_WRONG_MTH
 * @see ROUTING_WRONG_PATH
 **/
route_handler get_route_handler(char mth, char *path, int *status);

/**
 * Route a request, i.e. find the matching route and accordingly respond to the
 * request.
 * @param sock the socket which will be used to send the response
 * @param dz a pointer to the currently open dazibao
 * @param req the client request
 * @return 0 on success or -1 for a generic error, or an HTTP status for a more
 * specific error
 **/
int route_request(int sock, dz_t *dz, struct http_request *req);

/**
 * Free the routes table.
 * @return 0
 **/
int destroy_routes(void);

/**
 * Search for a file in PUBLIC_DIR which matches the given path an serve it if
 * there's one.
 * @param sock socket used for the response
 * @param req client request
 * @return 0 on success, -1 on failure
 **/
int file_response(int sock, struct http_request *req);

/**
 * Send an HTTP response 'resp' on the socket 'sock', and free all memory for
 * the 'resp' struct. Returns 0 on success, -1 on error.
 * @param sock socket
 * @param resp response to send
 * @return 0 on success, -1 on failure
 **/
int http_response(int sock, struct http_response *resp);

/**
 * Same as http_response, but let you choose if you want to free the 'resp'
 * struct with 'free_resp'.
 * @param sock socket
 * @param resp response to send
 * @param free_resp boolean flag. If set to 0, 'resp' won't be freed.
 * @return 0 on success, -1 on failure
 **/
int http_response2(int sock, struct http_response *resp, char free_resp);

/**
 * Send an HTTP error status ('status', as defined in http.h) in a socket
 * ('sock'). Returns 0 on success, -1 on error. If the error status is not
 * defined in http.h, a 400 (bad request) status is sent.
 * @param sock socket
 * @param status error status
 * @return 0 on success, -1 on failure
 **/
int error_response(int sock, int status);

#endif
