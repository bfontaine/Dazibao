#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include "dazibao.h"
#include "daziweb.h"
#include "utils.h"
#include "webutils.h"
#include "request.h"
#include "routing.h"
#include "routes.h"

static int listening_sock = -1;
static struct http_request *req;
static dz_t dz;

static void init_wserver_infos(void);
static void destroy_wserver_infos(void);

/**
 * Handler for signals used to close the listening socket.
 **/
static void clean_close(int s) {
        /* avoid 'unused parameter' warning */ s++;
        if (listening_sock >= 0 && close(listening_sock) == -1) {
            perror("close");
        }
        if (dz > 0) {
                dz_close(&dz);
        }
        destroy_wserver_infos();
        destroy_http_request(req);
        destroy_routes();
        exit(EXIT_SUCCESS);
}


/* -p <port> -l <loglevel> -d <dazibao path>
 * -v: if -l is not used, increase verbosity */
int parse_args(int argc, char **argv, int *port) {
        int l;
        char c,
             loglvl_flag = 0;

        dz = -1;
        WSERVER.dzname = NULL;
        WSERVER.dzpath = NULL;
        WSERVER.debug = 0;
        while ((c = getopt(argc, argv, "l:p:d:vD")) != -1) {
                switch (c) {
                case 'l':
                        l = strtol(optarg, NULL, 10);
                        if (!STRTOL_ERR(l)) {
                                _log_level = l;
                                loglvl_flag = 1;
                        }
                        break;
                case 'p':
                        *port = strtol(optarg, NULL, 10);
                        if (STRTOL_ERR(*port)) {
                                WLOGWARN("Wrong port: '%s'", optarg);
                                *port = DEFAULT_PORT;
                        }
                        WSERVER.port = *port;
                        break;
                case 'd':
                        if (dz == -1) {
                                if (dz_open(&dz,  optarg,  0) < 0) {
                                        WLOGFATAL("Cannot open '%s'", optarg);
                                        return -1;
                                }
                                dz_close(&dz);
                                dz = -2;
                                WSERVER.dzpath = strdup(optarg);
                                WSERVER.dzname = strdup(basename(optarg));
                        }
                        break;
                case 'D':
                        WSERVER.debug = 1;
                        break;
                case 'v':
                        if (!loglvl_flag) {
                                _log_level += 10;
                        }
                        break;
                case ':':
                        WLOGFATAL("-%c requires an argument", optopt);
                        return -1;
                case '?':
                        WLOGWARN("Unrecognized option: '-%c'", optopt);
                }
        }
        return 0;
}

/**
 * helper to move code off the main function
 **/
static void init_wserver_infos(void) {
        WSERVER.hostname = strdup("localhost"); /* should be ok for now */
        WSERVER.name = strdup("Daziweb/" DAZIWEB_VERSION);

        if (WSERVER.dzname == NULL) {
                WSERVER.dzname = strdup("<no dazibao>");
        } else {
                /* Remove the extension if there's one */
                char *tmp;
                for (int i=0; WSERVER.dzname[i] != '\0'; i++) {
                        if (WSERVER.dzname[i] == '.') {
                                tmp = (char*)malloc(sizeof(char)*(i+1));
                                strncpy(tmp, WSERVER.dzname, i);
                                tmp[i] = '\0';
                                free(WSERVER.dzname);
                                WSERVER.dzname = tmp;
                                break;
                        }
                }

                /* Capitalize the first word */
                if (WSERVER.dzname[0] >= 97 && WSERVER.dzname[0] <= 122) {
                        WSERVER.dzname[0] -= ('a' - 'A');
                }
        }
}

/** Helper to free the memory of WSERVER */
static void destroy_wserver_infos(void) {
        free(WSERVER.hostname);
        free(WSERVER.dzname);
        free(WSERVER.dzpath);
        free(WSERVER.name);
}

int main(int argc, char **argv) {
        int status = 0,
            port = DEFAULT_PORT;

        struct sockaddr_in bindaddr,
                           addr;
        socklen_t len = sizeof(struct sockaddr_in);
        struct sigaction sig;

        if (parse_args(argc, argv, &port) != 0) {
                if (dz > 0) {
                        dz_close(&dz);
                }
                exit(EXIT_FAILURE);
        }

        if (dz == -1) {
                WLOGFATAL("Starting with no dazibao");
                return -1;
        }
        init_wserver_infos();

        memset(&sig, 0, sizeof(sig));
        sig.sa_handler = clean_close;
        sig.sa_flags = 0;

        if (sigaction(SIGINT, &sig, NULL) == -1) {
                perror("sigaction");
                WLOGWARN("Cannot intercept SIGINT");
        }
        if (sigaction(SIGSEGV, &sig, NULL) == -1) {
                perror("sigaction");
                WLOGWARN("Cannot intercept SIGSEGV");
        }

        listening_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listening_sock == -1) {
                perror("socket");
                if (dz > 0) {
                        dz_close(&dz);
                }
                destroy_wserver_infos();
                exit(EXIT_FAILURE);
        }

        memset(&bindaddr, 0, sizeof(bindaddr));
        bindaddr.sin_family = AF_INET;
        bindaddr.sin_port = htons(port);
        bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (setsockopt(listening_sock, SOL_SOCKET,
                                SO_REUSEADDR, &status, sizeof(int)) == -1) {
                perror("setsockopt");
        }

        status = bind(listening_sock,
                        (struct sockaddr *)&bindaddr, sizeof(bindaddr));
        if (status == -1) {
                perror("bind");
                close(listening_sock);
                clean_close(0);
        }

        if (listen(listening_sock, MAX_QUEUE) == -1) {
                perror("listen");
                close(listening_sock);
                if (dz > 0) {
                        dz_close(&dz);
                }
                exit(EXIT_FAILURE);
        }

        if (WSERVER.debug) {
                WLOGINFO("Serving files in debug mode");
        }

        WLOGINFO("Listening on port %d...", port);
        WLOGINFO("Press ^C to interrupt.");

        register_routes();

        req = create_http_request();

        while (1) {
                int client;
                reset_http_request(req);

                if ((client = accept(listening_sock,
                                (struct sockaddr *)&addr, &len)) == -1) {
                        perror("accept");
                        continue;
                }
                WLOGINFO("Got a connection.");
                status = parse_request(client, req);
                if (status != 0) {
                        WLOGWARN("request parse error (status=%d)", status);
                        WLOGWARN("with method %d, path %s, body length %d",
                                req->method, req->path, req->body_len);
                        if (error_response(client, status) < 0) {
                                WLOGERROR("error_response failed");
                        }
                        WLOGINFO("Connection closed.");
                        if (close(client) == -1) {
                            perror("close");
                        }
                        if (dz > 0 && dz_close(&dz) == -1) {
                                WLOGERROR("Cannot close the Dazibao.");
                        }
                        continue;
                }
                WLOGDEBUG("Got method %d, path %s, body length %d",
                               req->method, req->path, req->body_len);
                WLOGDEBUG("User-Agent: %s", REQ_HEADER(*req, HTTP_H_UA));

                if (dz < 0 && dz_open(&dz, WSERVER.dzpath, 0) == -1) {
                        WLOGERROR("Cannot open the Dazibao.");
                }
                status = route_request(client, dz, req);
                dz_close(&dz);
                dz = -1;

                if (status != 0) {
                        WLOGWARN("route error, status=%d", status);
                        if (error_response(client, HTTP_S_NOTFOUND) < 0) {
                                WLOGERROR("404 error response failed");
                        }
                }

                WLOGINFO("Connection closed.");
                if (close(client) == -1) {
                        perror("close");
                }
        }

        WLOGINFO("Closing...");
        clean_close(0);
}
