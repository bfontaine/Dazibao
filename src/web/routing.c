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

int http_response(int sock, int status, struct http_headers *hs, char *body,
                        int bodylen) {
        const char *phrase = get_http_status_phrase(&status);
        char *response,
             *response2;
        int ret = 0, len,
            nobody = (body == NULL),
            noheaders = (hs == NULL);

        if (sock < 0) {
                return -1;
        }
        if (nobody || bodylen < 0) {
                bodylen = 0;
        }
        if (noheaders) {
                hs = (struct http_headers*)malloc(sizeof(struct http_header));
                if (hs == NULL) {
                        perror("malloc");
                        return -1;
                }
                http_init_headers(hs);
        }

        http_add_header(hs, "Allow", "GET,POST", 0);

        if (!nobody) {
                char ct[8];
                snprintf(ct, 7, "%d", bodylen);
                http_add_header(hs, "Content-Length", ct, 0);
        }

        /* 15 = strlen("HTTP/1.x xxx ") + strlen("\r\n") */
        len = 15+strlen(phrase);
        response = (char*)malloc(sizeof(char)*(len+1));
        if (response == NULL) {
                return -1;
        }

        /* status line (header) */
        if (snprintf(response, len+1, "HTTP/1.0 %3d %s\r\n",
                        status, phrase) < len) {
                WLOG("response sprintf failed");
                perror("sprintf");
                ret = -1;
        }

        if (!noheaders) {
                char *str_headers = http_headers_string(hs);
                int len_headers = strlen(str_headers);
                len += len_headers + 2;
                response2 = realloc(response, len);
                if (response2 == NULL) {

                }
                response = response2;

                strncat(response, str_headers, len_headers);
                strncat(response, "\r\n", 2);

                write(sock, response, len);

                NFREE(str_headers);
                http_destroy_headers(hs);
        }

        /* TODO send body */

        NFREE(response);
        return ret;
}

int error_response(int sock, int status) {
        return http_response(sock, status, NULL, NULL, -1);
}
