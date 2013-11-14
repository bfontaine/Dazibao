#include <stdio.h>
#include <string.h>
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
