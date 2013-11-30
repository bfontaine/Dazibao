#include <stdio.h>
#include <string.h>
#include "routes.h"
#include "webutils.h"
#include "routing.h"
#include "dazibao.h"
#include "http.h"
#include "html.h"

int route_get_index(dz_t dz, struct http_request req,
                        int *status, char **resp, int *resplen) {

        if (strcmp(req.path, "/index.html") != 0 || dz <= 0
                        || req.method != HTTP_M_GET) {
                WLOGERROR("get_index - got wrong path '%s' and/or wrong " \
                                "dz=%d and/or wrong method %d.",
                                req.path, dz, req.method);
                return -1;
        }

        if (dz2html(dz, resp) < 0) {
                WLOGERROR("Error while making dazibao's HTML");
                return -1;
        }

        *resplen = strlen(*resp);
        *status = HTTP_S_OK;

        return 0;
}

int register_routes(void) {
        int st = 0;

        /* Add routes here */
        st |= add_route(HTTP_M_GET, "/index.html", route_get_index);
        return st;
}
