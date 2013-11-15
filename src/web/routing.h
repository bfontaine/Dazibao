#ifndef _ROUTING_H
#define _ROUTING_H 1

#define MAX_ROUTES 16

/**
 * Type of a route handler. This a pointer to a function which returns
 * an int (a status, 0 if everything is ok, or an error code if there was an
 * error. It takes the following arguments:
 *  - method (int): the method used by the request, as defined in http.h
 *  - path (char*): the path of the request (e.g. /index.html)
 *  - body (char*): the raw body of the request
 *  - len (int): the length of this body
 *  - status (int*): this is a result variable. It should be filled by the
 *    function to represent the status of the response
 *  - resp (char*): this is a result variable. It should be filled with the
 *    actual response body
 *  - resplen (int*): this is a result variable. It should be filled with the
 *    length of the response body
 **/
typedef int (*route_handler)(int, char*, char*, int, int*, char*, int*);

/**
 * Add a new route handler. 'path_suffix' is a string which will be used to
 * match an URL path. 'route' is a pointer on a route handler. The function
 * returns 0 on success, and -1 on error.
 **/
int add_route(char *path_suffix, route_handler route);

/**
 * Return a route handler for a path.
 **/
route_handler get_route_handler(char *path);

/**
 * Send an HTTP error status ('status', as defined in http.h) in a socket
 * ('sock'). Returns 0 on success, -1 on error. If the error status is not
 * defined in http.h, a 400 (bad request) status is sent.
 **/
int error_response(int sock, int status);

#endif
