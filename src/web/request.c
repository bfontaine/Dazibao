#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "web/request.h"
#include "utils.h"

char is_crlf(char *s, int c, int len) {
        return s != NULL && c < len-1 && s[c] == CR && s[c+1] == LF;
}

char *next_header(int sock, int *eoh) {
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
                restlen = 0;
        } else {
                len = recv(sock, buff, BUFFLEN, 0);

                if (len < 0) {
                        perror("recv");
                        return NULL;
                }
                if (len == 0) {
                        return NULL;
                }
        }

        line = (char*)malloc(sizeof(char)*len+1);
        i = 0; /* line cursor */
        j = 0; /* buffer cursor */
        in_crlf = 0; /* if in the middle of an CRLF */

        while (1) {
                for (j=0; j<len; j++) {
                        if (in_crlf) {
                                if (buff[j] != LF) {
                                        line[i++] = CR;
                                } else {
                                        restlen = len-j-1;
                                        if (memcpy(rest, buff+j,
                                                        restlen) == NULL) {
                                                perror("memcpy");
                                        }
                                        goto RETURN_LINE;
                                }
                                in_crlf = 0;
                        }

                        if (is_crlf(buff, j, len)) {
                                restlen = len-j-2;
                                if (memcpy(rest, buff+j+2, restlen) == NULL) {
                                        perror("memcpy");
                                }
                                goto RETURN_LINE;
                        }
                        if (buff[j] == CR && j+1 == len) {
                                in_crlf = 1;
                                break; /* same than 'continue' here */
                        }
                        line[i++] = buff[j];
                }
                /* at this point we read the whole buffer but haven't
                   encountered a CRLF */
                len = recv(sock, buff, BUFFLEN, 0);
                if (len <= 0) {
                        if (len < 0) {
                                perror("read");
                        }
                        restlen = 0;
                        goto RETURN_LINE;
                }

                line2 = realloc(line, i+BUFFLEN);

                if (line2 == NULL) {
                        perror("realloc");
                        NFREE(line);
                        restlen = 0;
                        return NULL;
                }
                line = line2;

        };

        restlen = 0;
        return NULL;

RETURN_LINE:
        if (i == 0) {
                if (memcpy(line, buff, len) == NULL) {
                        perror("memcpy");
                }
                *eoh = len;
                return line;
        }
        line[i] = '\0';
        return line;
}

void parse_request(int sock) {
        /* TODO */
}

