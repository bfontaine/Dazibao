#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include "daziweb.h"
#include "webutils.h"
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
        int port, status = 0,
            mth, body_len;
        char *path = NULL;
        char *body = NULL;

        /* FIXME 'a' is not an understandable name */
        struct sockaddr_in a,
                           addr;
        socklen_t len = sizeof(struct sockaddr_in);
        struct sigaction sig;

        sig.sa_handler = clean_close;
        sig.sa_sigaction = NULL;
        sig.sa_flags = 0;
        sigemptyset(&sig.sa_mask);
        sigaddset(&sig.sa_mask, SIGINT);

        if (sigaction(SIGINT, &sig, NULL) == -1) {
                perror("sigaction");
        }

        if (argc < 2) {
                port = DEFAULT_PORT;
                WLOGINFO("Using default port");
        } else {
                port = strtol(argv[1], NULL, 10);
                if (port == LONG_MIN || port == LONG_MAX) {
                        WLOGDEBUG("trouble parsing the port, using default");
                        port = DEFAULT_PORT;
                }
        }

        listening_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listening_sock == -1) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        bzero(&a, sizeof(struct sockaddr_in));
        a.sin_family = AF_INET;
        a.sin_port = htons(port);
        a.sin_addr.s_addr = htonl(INADDR_ANY);

        if (setsockopt(listening_sock, SOL_SOCKET,
                                SO_REUSEADDR, &status, sizeof(int)) == -1) {
                perror("setsockopt");
        }

        status = bind(listening_sock,
                        (struct sockaddr *)&a, sizeof(struct sockaddr_in));
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
        if (close(listening_sock) == -1) {
                perror("close");
        }
        exit(EXIT_SUCCESS);
}
