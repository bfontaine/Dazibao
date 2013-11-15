#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"
#include "webutils.h"
#include "http.h"
#include "routing.h"

static char *routes_paths[MAX_ROUTES];
static route_handler routes_handlers[MAX_ROUTES];

static int routes_cpt = 0;

int add_route(char *path_suffix, route_handler route) {
    if (routes_cpt >= MAX_ROUTES) {
        return -1;
    }
    routes_paths[routes_cpt] = path_suffix;
    routes_handlers[routes_cpt] = route;
    routes_cpt++;

    return 0;
}

route_handler get_route_handler(char *path) {

    for (int i=0, len; i<routes_cpt; i++) {
        len = strlen(routes_paths[i]);
        if (strncmp(path, routes_paths[i], len) == 0) {
            return routes_handlers[i];
        }
    }

    return NULL;
}

/*
 * TODO: generic http_response function, something like:
 *   http_response(int sock, int status, Hs, B)
 *  where:
 *      Hs is a possibly empty list of headers to add to the response
 *      B: is the response body (NULLable)
 */

int error_response(int sock, int status) {
        const char *phrase = get_http_status_phrase(&status);
        char *response;
        int ret = 0, len;

        if (sock < 0) {
                return -1;
        }

        /* 15 = strlen("HTTP/1.0 xxx \r\n") */
        len = 15+strlen(phrase);
        response = (char*)malloc(sizeof(char)*(len+1));
        if (response == NULL) {
                return -1;
        }

        WLOG("Sending error response.");

        if (snprintf(response, len+1, "HTTP/1.0 %3d %s\r\n",
                        status, phrase) < len) {
                WLOG("Error response sprintf failed");
                perror("sprintf");
                ret = -1;

        } else if (write(sock, response, len) <= 0) {
                WLOG("Cannot send an error response (status=%d)", status);
                perror("send");
                ret = -1;
        }

        NFREE(response);
        return ret;
}
