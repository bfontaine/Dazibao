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

char is_crlf(char *s, int c, int len) {
        return s != NULL && c < len-1 && s[c] == CR && s[c+1] == LF;
}

char *next_header(int sock) { /* FIXME */
        static char rest[BUFFLEN];
        static int restlen = 0;

        char buff[BUFFLEN];
        char *line,
             *line2;
        int len, i, j,
            in_crlf;

        if (restlen > 0) {
                if (memcpy(buff, rest, restlen) == NULL) {
                        perror("memcpy");
                        return NULL;
                }
                len = restlen;
        } else {
                len = recv(sock, buff, BUFFLEN, 0);

                if (len < 0) {
                        perror("recv");
                        return NULL;
                }
                if (len == 0) {
                        return strdup("");
                }
        }

        line = (char*)malloc(sizeof(char)*BUFFLEN+1);
        i = 0; /* line cursor */
        j = 0; /* buffer cursor */
        in_crlf = 0; /* if in the middle of an CRLF */

        do {
                for (j=0; j<len; j++) {
                        if (in_crlf) {
                             if (buff[j] != LF) {
                                     line[i++] = CR;
                             } else {
                                     restlen = len-j;
                                     if (memcpy(rest, buff+j,
                                                restlen) == NULL) {
                                             perror("memcpy");
                                     }
                                     line[i] = '\0';
                                     return line;
                             }
                             in_crlf = 0;
                        }

                        if (is_crlf(buff, j, len)) {
                                restlen = len-j;
                                if (memcpy(rest, buff+j, restlen) == NULL) {
                                        perror("memcpy");
                                }
                                return line;
                        }
                        if (buff[j] == CR && j+1 == len) {
                                in_crlf = 1;
                                break;
                        }
                        line[i++] = buff[j++];
                }
                /* at this point we read the whole buffer but haven't
                   encountered a CRLF */
                len = recv(sock, buff, BUFFLEN, 0);
                if (len <= 0) {
                        if (len < 0) {
                                perror("read");
                        }
                        line[i] = '\0';
                        restlen = 0;
                        return line;
                }

                line2 = realloc(line, i+BUFFLEN);

                if (line2 == NULL) {
                        perror("realloc");
                        NFREE(line);
                        restlen = 0;
                        return NULL;
                }
                line = line2;

        } while(1);

        restlen = 0;
        return NULL;
}

int main(int argc, char *argv[]) {
        int port, status,
            d, i;
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
                        /*
                        close(s);
                        exit(EXIT_FAILURE);
                        */
                }
                /*shutdown(s,SHUT_WR);*/

                while ((line = next_header(d)) != NULL) {
                        /* test */
                        printf("%s\n", line);
                        NFREE(line);
                }
                /*

                while ((status = recv(d, buff, BUFFLEN, 0)) > 0) {
                        / * test * /
                        for (i=0; i<status; i++) {
                                if (buff[i] == CR
                                        && i < status-1 && buff[i] == LF) {
                                        printf("\n");
                                        continue;
                                } else {
                                        printf("%c", buff[i]);
                                }
                        }
                }
                if (status == -1) {
                        perror("read");
                }*/

                close(d);
        };
        close(listening_sock);
        exit(EXIT_SUCCESS);
}
