#include <stdio.h>
#include <string.h>
#include "routes.h"
#include "webutils.h"
#include "routing.h"
#include "dazibao.h"
#include "http.h"
#include "html.h"

int route_get_index(dz_t dz, int mth, char *path, char *body, int bodylen,
                        int *status, char **resp, int *resplen) {

        if (strcmp(path, "/index.html") != 0 || dz <= 0 || mth != HTTP_M_GET) {
                WLOGERROR("get_index - got wrong path '%s' and/or wrong " \
                                "dz=%d and/or wrong method %d.",
                                path, dz, mth);
                return -1;
        }

        if (body != NULL && bodylen > 0) {
                WLOGDEBUG("GET /index got a body (len=%d)", bodylen);
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
