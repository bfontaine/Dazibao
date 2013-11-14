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

int listening_sock;

void clean_close(int s) {
        /* to avoid unused parameter warning: */ s++;
        if (close(listening_sock) == -1) {
            perror("close");
        }
        exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
        int port, status,
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
                WLOG("Using default port");
        } else {
                port = strtol(argv[1], NULL, 10);
                if (port == LONG_MIN || port == LONG_MAX) {
                        WLOG("trouble parsing the port, using default");
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

        WLOG("Listening on port %d...", port);
        WLOG("Press ^C to interrupt.");

        while (1) {
                int d;

                if ((d = accept(listening_sock,
                                (struct sockaddr *)&addr, &len)) == -1) {
                        perror("accept");
                        continue;
                }
                WLOG("Got a connection.");

                if (parse_request(d, &mth, &path, &body, &body_len) < 0) {
                    /* TODO return error 400 */
                    WLOG("request parse error");
                    if (close(d) == -1) {
                        perror("close");
                    }
                    continue;
                }

                WLOG("Got method %d, path %s, body length %d\n",
                                mth, path, body_len);

                /* TODO respond to the request */

                WLOG("Connection closed.");
                if (close(d) == -1) {
                    perror("close");
                }
        };

        WLOG("Closing...");
        if (close(listening_sock) == -1) {
            perror("close");
        }
        exit(EXIT_SUCCESS);
}
