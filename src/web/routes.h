#ifndef _ROUTES_H
#define _ROUTES_H 1

#include "dazibao.h"
#include "request.h"

/* Remember, a route handler is defined as:
 *   int f(dz,
 *         req
 *         status_result, &body_result, body_length_result)
 *
 * Template:
int route_foobar(dz_t dz, struct http_request req,
                        int *status, char **resp, int *resplen);
 *
 */

/**
 * Route for /index.* paths
 **/
int route_index(dz_t dz, struct http_request req,
                        int *status, char **resp, int *resplen);


/**
 * Register all the above routes.
 **/
int register_routes(void);

#endif
