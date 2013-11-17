#include <stdio.h>
#include <string.h>
#include "routes.h"
#include "webutils.h"
#include "routing.h"
#include "dazibao.h"
#include "http.h"

int route_get_index(dz_t dz, int mth, char *path, char *body, int bodylen,
                        int *status, char **resp, int *resplen) {

        if (strcmp(path, "/index") != 0 || dz <= 0 || mth != HTTP_M_GET) {
                return -1;
        }

        if (body != NULL && bodylen > 0) {
                WLOGDEBUG("GET /index got a body (len=%d)", bodylen);
        }

        /* TODO parse dazibao and return html repr */
        *status = HTTP_S_NOTIMPL;
        *resp = strdup("Not implemented...\r\n");
        *resplen = strlen(*resp)+1; /* TODO check why +1 is needed here */

        return 0;
}

int register_routes(void) {
        int st = 0;

        /* Add routes here */
        st |= add_route(HTTP_M_GET, "/index", route_get_index);

        return st;
}
