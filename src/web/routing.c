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
 routes_paths, and because a method is defined as a number below 127 in http.h,
 we replace the first character of the path, which is always a slash ('/') with
 this number.
 */

int add_route(char mth, char *path_suffix, route_handler route) {
        if (routes_cpt >= MAX_ROUTES) {
                return -1;
        }
        if (path_suffix == NULL || path_suffix[0] != '/' || mth == 0) {
                return -1;
        }

        WLOGDEBUG("Route handler for method %d and suffix '%s' added.",
                        mth, path_suffix);

        routes_paths[routes_cpt] = strdup(path_suffix);
        routes_paths[routes_cpt][0] = mth;


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
        for (int i=0; i<routes_cpt; i++) {
                NFREE(routes_paths[i]);
        }

        routes_cpt = 0;
        return 0;
}

int route_request(int sock, dz_t dz, struct http_request *req) {
        route_handler rh;
        char *resp = NULL;
        int status = -1,
            rst; /* route status */

        struct http_response *respst;

        if (req->body == NULL) {
                req->body_len = -1;
        }

        if (sock < 0) {
                WLOGERROR("Cannot route a request on negative socket");
                return -1;
        }

        if (req->path == NULL) {
                WLOGERROR("Got a NULL path");
                error_response(sock, HTTP_S_BADREQ);
                return -1;
        }

        if (req->method == HTTP_M_UNSUPPORTED) {
                error_response(sock, HTTP_S_NOTIMPL);
                return 0;
        }

        if (dz < 0) {
                WLOGWARN("Routing a request with no dazibao (%d)", dz);
        }

        rh = get_route_handler(req->method, req->path);

        if (rh == NULL) {
                WLOGWARN("No route found for path '%s'", req->path);
                error_response(sock, HTTP_S_NOTFOUND);
                return 0;
        }

        if (req->method == HTTP_M_POST && req->body_len <= 0) {
                WLOGWARN("Got a POST request with empty body on '%s'",
                                req->path);
        }

        respst = create_http_response();
        if (respst == NULL) {
                WLOGERROR("Cannot create a new HTTP response");
                NFREE(resp);
                error_response(sock, HTTP_S_ERR);
                return -1;
        }

        rst = (*rh)(dz, *req, respst);
        if (rst < 0) {
                WLOGERROR("Route handler error, rst=%d, status=%d",
                                rst, status);
                destroy_http_response(respst);
                NFREE(resp);
                return -1;
        }

        status = http_response(sock, respst);

        NFREE(resp);
        return status;
}

int http_response(int sock, struct http_response *resp) {
        const char *phrase = get_http_status_phrase(&resp->status);
        char *str_headers,
             *response;
        int ret = 0, len,
            no_body = (resp->body == NULL || *resp->body == NULL),
            noheaders = (resp->headers == NULL),
            len_headers;

        if (sock < 0) {
                return -1;
        }
        if (no_body || resp->body_len < 0) {
                resp->body_len = 0;
        }
        if (noheaders) {
                resp->headers = (struct http_headers*)malloc(
                                        sizeof(struct http_headers));
                if (resp->headers == NULL) {
                        perror("malloc");
                        return -1;
                }
                http_init_headers(resp->headers);
        }

        if (resp->status == HTTP_S_NOTALLOWED) {
                http_add_header(resp->headers, HTTP_H_ALLOW, "GET,POST", 0);
        }

        if (!no_body) {
                char ct[16];
                snprintf(ct, 15, "%d", resp->body_len);
                http_add_header(resp->headers, HTTP_H_CONTENT_LENGTH, ct, 0);
        }

        /* HTTP/1.x <code> <phrase>\r\n */
        len = strlen("HTTP/") + strlen(HTTP_VERSION) + 5 + strlen(phrase) + 2;
        response = (char*)malloc(sizeof(char)*(len+1));
        if (response == NULL) {
                perror("malloc");
                ret = -1;
                goto EORESP;
        }

        /* status line (header) */
        if (snprintf(response, len+1, "HTTP/" HTTP_VERSION " %3d %s\r\n",
                        resp->status, phrase) < len) {
                WLOGERROR("response sprintf failed");
                perror("sprintf");
                ret = -1;
        }

        /* -- headers -- */
        str_headers = http_headers_string(resp->headers);
        if (str_headers == NULL) {
                WLOGERROR("Cannot get headers string");
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
                ret = -1;
                goto EORESP;
        }

        strncat(response, str_headers, len_headers);
        strncat(response, "\r\n", 2);

        if (write_all(sock, response, len) < len) {
            WLOGERROR("Couldn't send all headers (len=%d)", len);
        }

        NFREE(str_headers);
        /* -- /headers -- */

        if (write_all(sock, *resp->body, resp->body_len) < resp->body_len) {
            WLOGERROR("Couldn't send the whole request body (len=%d)",
                            resp->body_len)
        }

EORESP:
        destroy_http_response(resp);
        NFREE(response);
        return ret;
}

int error_response(int sock, int status) {
        struct http_response *resp = create_http_response();

        resp->status = status;
        return http_response(sock, resp);
}
