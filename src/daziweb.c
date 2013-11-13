#include "daziweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <unistd.h>

/* TODO trap sigint to terminate properly */

int main(int argc, char *argv[]) {
        int port, status, s,
            d, i;
        struct sockaddr_in a,
                           addr;
        socklen_t len;
        char buff[BUFFLEN];

        if (argc < 2) {
                port = DEFAULT_PORT;
        } else {
                port = strtol(argv[1], NULL, 10);
                if (port == LONG_MIN || port == LONG_MAX) {
                        port = DEFAULT_PORT;
                }
        }

        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == -1) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        bzero(&a, sizeof(struct sockaddr_in));
        a.sin_family = AF_INET;
        a.sin_port = htons(port);
        a.sin_addr.s_addr = htonl(INADDR_ANY);

        status = bind(s, (struct sockaddr *)&a, sizeof(struct sockaddr_in));
        
        if (status == -1) {
                perror("bind");
                close(s);
                exit(EXIT_FAILURE);    
        }

        if (listen(s, MAX_QUEUE) == -1) {
                perror("listen");
                close(s);
                exit(EXIT_FAILURE);
        }

        printf("Listening on port %d...\n", port);

        while (1) {
                if ((d = accept(s, (struct sockaddr *)&addr, &len)) == -1) {
                        perror("accept");
                        /*
                        close(s);
                        exit(EXIT_FAILURE);
                        */
                }
                /*shutdown(s,SHUT_WR);*/

                while ((status = read(d, buff, BUFFLEN)) > 0) {
                        // test
                        for (i=0; i<status; i++)
                                printf("%2x(%c) ", buff[i], buff[i]);
                        printf("\n");
                }
                if (status == -1) {
                        perror("read");
                }

                close(d);
        };
        close(s);
        exit(EXIT_SUCCESS);
}
