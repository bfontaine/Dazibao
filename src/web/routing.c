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

/*
 The following two functions use a little trick to match a request on both its
 path and its method. Paths are stored as NULL-terminated strings in
 routes_paths, and since a method is defined as a number below 127 in http.h,
 we replace the first character of the path, which is always a slash ('/') with
 this number.
 */

int add_route(char mth, char *path_suffix, route_handler route) {
        char *p2;

        if (routes_cpt >= MAX_ROUTES) {
                return -1;
        }
        if (path_suffix[0] != '/' || mth == 0) {
                return -1;
        }

        WLOGDEBUG("Route handler for method %d and suffix '%s' added.",
                        mth, path_suffix);

        p2 = strdup(path_suffix);
        p2[0] = mth;

        routes_paths[routes_cpt] = p2;
        routes_handlers[routes_cpt] = route;
        routes_cpt++;

        return 0;
}

route_handler get_route_handler(char mth, char *path) {
        WLOGDEBUG("Getting route handler for method %d, path '%s'", mth, path);

        for (int i=0, len, pmth, paths_match; i<routes_cpt; i++) {
                pmth = routes_paths[i][0];

                if ((mth & pmth) != mth) {
                        continue;
                }

                routes_paths[i][0] = '/';
                len = strlen(routes_paths[i]);
                paths_match = (strncmp(path, routes_paths[i], len) == 0);
                routes_paths[i][0] = pmth;

                if (paths_match) {
                        return routes_handlers[i];
                }
        }

        return NULL;
}

int destroy_routes(void) {
        if (routes_cpt == 0) {
                return 0;
        }

        for (int i=0; i<routes_cpt; i++) {
                NFREE(routes_paths[i]);
        }

        routes_cpt = 0;
        return 0;
}

int route_request(int sock, dz_t dz, int mth, char *path, char *body,
                        int bodylen) {
        route_handler rh;
        char *resp = NULL;
        int resplen = -1,
            status = -1,
            rstatus = -1,
            rst; /* route status */

        if (body == NULL) {
                bodylen = -1;
        }

        if (sock < 0) {
                WLOGERROR("Cannot route a request on negative socket");
                return -1;
        }

        if (path == NULL) {
                WLOGERROR("Got a NULL path");
                error_response(sock, HTTP_S_BADREQ);
                return -1;
        }

        if (mth == HTTP_M_UNSUPPORTED) {
                error_response(sock, HTTP_S_NOTIMPL);
                return 0;
        }

        if (dz < 0) {
                WLOGWARN("Routing a request with no dazibao (%d)", dz);
        }

        rh = get_route_handler(mth, path);

        if (rh == NULL) {
                WLOGWARN("No route found for path '%s'", path);
                error_response(sock, HTTP_S_NOTFOUND);
                return 0;
        }

        if (mth == HTTP_M_POST && bodylen <= 0) {
                WLOGWARN("Got a POST request with empty body on '%s'", path);
        }

        rst = (*rh)(dz, mth, path, body, bodylen, &rstatus, &resp, &resplen);
        if (rst < 0) {
                WLOGERROR("Route handler error, rst=%d, status=%d",
                                rst, status);
                NFREE(resp);
                return -1;
        }

        status = http_response(sock, rstatus, NULL, resp, resplen);

        NFREE(resp);
        return status;
}

int http_response(int sock, int status, struct http_headers *hs, char *body,
                        int bodylen) {
        const char *phrase = get_http_status_phrase(&status);
        char *response,
             *str_headers;
        int ret = 0, len,
            no_body = (body == NULL),
            noheaders = (hs == NULL),
            len_headers;

        if (sock < 0) {
                return -1;
        }
        if (no_body || bodylen < 0) {
                bodylen = 0;
        }
        if (noheaders) {
                hs = (struct http_headers*)malloc(sizeof(struct http_headers));
                if (hs == NULL) {
                        perror("malloc");
                        return -1;
                }
                http_init_headers(hs);
        }

        /* TODO: add this header only with 405 Method Not Allowed */
        http_add_header(hs, HTTP_H_ALLOW, "GET,POST", 0);

        if (!no_body) {
                char ct[8];
                snprintf(ct, 7, "%d", bodylen);
                http_add_header(hs, HTTP_H_CONTENT_LENGTH, ct, 0);
        }

        /* 15 = strlen("HTTP/1.x xxx ") + strlen("\r\n") */
        len = 15+strlen(phrase);
        response = (char*)malloc(sizeof(char)*(len+1));
        if (response == NULL) {
                http_destroy_headers(hs);
                return -1;
        }

        /* status line (header) */
        if (snprintf(response, len+1, "HTTP/1.0 %3d %s\r\n",
                        status, phrase) < len) {
                WLOGERROR("response sprintf failed");
                perror("sprintf");
                ret = -1;
        }

        /* -- headers -- */
        str_headers = http_headers_string(hs);
        if (str_headers == NULL) {
                WLOGERROR("Cannot get headers string");
                http_destroy_headers(hs);
                ret = -1;
                goto EORESP;
        }
        len_headers = strlen(str_headers);
        len += len_headers + 3;
        response = safe_realloc(response, len);
        if (response == NULL) {
                WLOGERROR("Cannot realloc for headers");
                perror("realloc");
                NFREE(str_headers);
                http_destroy_headers(hs);
                ret = -1;
                goto EORESP;
        }

        strncat(response, str_headers, len_headers);
        strncat(response, "\r\n", 2);

        if (write_all(sock, response, len) < len) {
            WLOGERROR("Couldn't send all headers (len=%d)", len);
        }

        NFREE(str_headers);
        http_destroy_headers(hs);
        /* -- /headers -- */

        if (write_all(sock, body, bodylen) < bodylen) {
            WLOGERROR("Couldn't send the whole request body (len=%d)", bodylen)
        }

EORESP:
        NFREE(response);
        return ret;
}

int error_response(int sock, int status) {
        return http_response(sock, status, NULL, NULL, -1);
}
