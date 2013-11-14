#include "daziweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>

int listening_sock;

void clean_close(int s) {
        /* to avoid unused parameter warning: */ s++;
        close(listening_sock);
}

int main(int argc, char *argv[]) {
        int port, status,
            d, i, eoh;
        struct sockaddr_in a,
                           addr;
        socklen_t len = sizeof(struct sockaddr_in);
        char buff[BUFFLEN];
        char *line = NULL;
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
        } else {
                port = strtol(argv[1], NULL, 10);
                if (port == LONG_MIN || port == LONG_MAX) {
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

        printf("Listening on port %d...\n", port);
        puts("Press ^C to interrupt.");

        while (1) {
                if ((d = accept(listening_sock,
                                (struct sockaddr *)&addr, &len)) == -1) {
                        perror("accept");
                        continue;
                }

                eoh = 0;

                /* <test> */
                while ((line = next_header(d, &eoh)) != NULL) {
                        if (!eoh) {
                                printf("%s\n", line);
                        }
                        NFREE(line);
                        if (eoh) {
                                printf("-- end of headers --\n");
                                break;
                        }
                }
                /* </test> */

                close(d);
        };
        close(listening_sock);
        exit(EXIT_SUCCESS);
}
