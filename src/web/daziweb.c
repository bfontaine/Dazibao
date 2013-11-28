#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include "dazibao.h"
#include "daziweb.h"
#include "utils.h"
#include "webutils.h"
#include "request.h"
#include "routing.h"
#include "routes.h"

static int listening_sock;
static dz_t dz;

/* FIXME: this function is not called on ^C
   (I checked with Valgrind, GDB and strace) */
void clean_close(int s) {
        /* avoid 'unused parameter' warning */ s++;
        if (close(listening_sock) == -1) {
            perror("close");
        }
        if (dz > 0) {
                dz_close(&dz);
        }
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
        while ((c = getopt(argc, argv, "l:p:d:v")) != -1) {
                switch (c) {
                case 'l':
                        l = strtol(optarg, NULL, 10);
                        if (!STRTOL_ERR(l)) {
                                _wlog_level = l;
                                loglvl_flag = 1;
                        }
                        break;
                case 'p':
                        *port = strtol(optarg, NULL, 10);
                        if (STRTOL_ERR(*port)) {
                                WLOGWARN("Wrong port: '%s'", optarg);
                                *port = DEFAULT_PORT;
                        }
                        break;
                case 'd':
                        if (dz == -1 && dz_open(&dz,  optarg,  0) < 0) {
                                WLOGFATAL("Cannot open '%s'", optarg);
                                return -1;
                        }
                        break;
                case 'v':
                        if (!loglvl_flag) {
                                _wlog_level += 10;
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

int main(int argc, char **argv) {
        int status = 0,
            mth, body_len,
            port = DEFAULT_PORT;
        char *path = NULL;
        char *body = NULL;

        struct sockaddr_in bindaddr,
                           addr;
        socklen_t len = sizeof(struct sockaddr_in);
        struct sigaction sig;

        sig.sa_handler = clean_close;
        sig.sa_sigaction = NULL;
        sig.sa_flags = 0;
        sigemptyset(&sig.sa_mask);

        if (parse_args(argc, argv, &port) != 0) {
                if (dz > 0) {
                        dz_close(&dz);
                }
                exit(EXIT_FAILURE);
        }

        if (sigaction(SIGINT, &sig, NULL) == -1) {
                perror("sigaction");
        }

        if (dz < 0) {
                WLOGWARN("Starting with no dazibao");
        }

        listening_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listening_sock == -1) {
                perror("socket");
                if (dz > 0) {
                        dz_close(&dz);
                }
                exit(EXIT_FAILURE);
        }

        bzero(&bindaddr, sizeof(struct sockaddr_in));
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
                if (dz > 0) {
                        dz_close(&dz);
                }
                exit(EXIT_FAILURE);
        }

        if (listen(listening_sock, MAX_QUEUE) == -1) {
                perror("listen");
                close(listening_sock);
                if (dz > 0) {
                        dz_close(&dz);
                }
                exit(EXIT_FAILURE);
        }

        WLOGINFO("Listening on port %d...", port);
        WLOGINFO("Press ^C to interrupt.");

        register_routes();

        while (1) {
                int client;
                NFREE(path);
                NFREE(body);

                if ((client = accept(listening_sock,
                                (struct sockaddr *)&addr, &len)) == -1) {
                        perror("accept");
                        continue;
                }
                WLOGINFO("Got a connection.");

                status = parse_request(client, &mth, &path, &body, &body_len);
                if (status != 0) {
                        WLOGWARN("request parse error (status=%d)", status);
                        WLOGWARN("with method %d, path %s, body length %d",
                                mth, path, body_len);
                        if (error_response(client, status) < 0) {
                                WLOGERROR("error_response failed");
                        }
                        WLOGINFO("Connection closed.");
                        if (close(client) == -1) {
                            perror("close");
                        }
                        continue;
                }
                WLOGDEBUG("Got method %d, path %s, body length %d",
                               mth, path, body_len);
                status = route_request(client, dz, mth, path, body, body_len);
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

                if (dz_reset(&dz) < 0) {
                        WLOGWARN("Cannot reset dazibao.");
                }
        }

        WLOGINFO("Closing...");
        clean_close(0);
}
