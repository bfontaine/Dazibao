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

static int listening_sock;

void clean_close(int s) {
        /* avoid 'unused parameter' warning */ s++;
        if (close(listening_sock) == -1) {
            perror("close");
        }
        exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
        int status = 0, c, l, loglvlflag,
            mth, body_len,
            port = DEFAULT_PORT;
        char *path = NULL;
        char *body = NULL;

        struct sockaddr_in bindaddr,
                           addr;
        socklen_t len = sizeof(struct sockaddr_in);
        struct sigaction sig;

        dz_t dz;

        sig.sa_handler = clean_close;
        sig.sa_sigaction = NULL;
        sig.sa_flags = 0;
        sigemptyset(&sig.sa_mask);

        if (sigaction(SIGINT, &sig, NULL) == -1) {
                perror("sigaction");
        }

        /* -p <port> -l <loglevel> -d <dazibao path>
         * -v: if -l is not used, increase verbosity */
        port = DEFAULT_PORT;
        l = _wlog_level;
        loglvlflag = 0;
        dz = -1;
        while ((c = getopt(argc, argv, "l:p:d:")) != -1) {
                switch (c) {
                case 'l':
                        l = strtol(optarg, NULL, 10);
                        if (!STRTOL_ERR(l)) {
                                _wlog_level = l;
                                loglvlflag = 1;
                        }
                        break;
                case 'p':
                        port = strtol(optarg, NULL, 10);
                        if (STRTOL_ERR(port)) {
                                WLOGWARN("Wrong port: '%s'", optarg);
                                port = DEFAULT_PORT;
                        }
                        break;
                case 'd':
                        if (dz == -1 && dz_open(&dz,  optarg,  0) < 0) {
                                WLOGFATAL("Cannot open '%s'", optarg);
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'v':
                        if (!loglvlflag) {
                                l += 10;
                        }
                        break;
                case ':':
                        WLOGFATAL("-%c requires an argument", optopt);
                        exit(EXIT_FAILURE);
                case '?':
                        WLOGWARN("Unrecognized option: '-%c'", optopt);
                }
        }

        listening_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listening_sock == -1) {
                perror("socket");
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
                exit(EXIT_FAILURE);
        }

        if (listen(listening_sock, MAX_QUEUE) == -1) {
                perror("listen");
                close(listening_sock);
                exit(EXIT_FAILURE);
        }

        WLOGINFO("Listening on port %d...", port);
        WLOGINFO("Press ^C to interrupt.");

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
                        if (close(client) == -1) {
                            perror("close");
                        }
                        continue;
                }

                WLOGDEBUG("Got method %d, path %s, body length %d",
                               mth, path, body_len);

                /* TODO respond to the request */

                WLOGINFO("Connection closed.");
                if (close(client) == -1) {
                        perror("close");
                }
        };

        WLOGINFO("Closing...");
        clean_close(0);
}
