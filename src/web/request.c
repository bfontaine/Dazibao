#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "request.h"
#include "http.h"
#include "webutils.h"
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

        if (sock <= 0) {
                restlen = 0;
                return NULL;
        }

        if (eoh == NULL) {
                return NULL;
        }

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

int parse_request(int sock, int *mth, char **path, char **body, int *len) {
        char *line = NULL,
             *mth_str;

        int eoh = 0,
            readlen = 0,
            body_len = 0;

        *len = 0;

        if (*path != NULL) {
                NFREE(*path);
        }

        line = next_header(sock, &eoh);
        if (line == NULL || eoh) {
                WLOG("Cannot get the first header line (eoh=%d)", eoh);
                next_header(-1, NULL);
                return -1;
        }

        mth_str = (char*)malloc(sizeof(char)*(HTTP_MAX_MTH_LENGTH+1));
        if (mth_str == NULL) {
                perror("malloc");
                next_header(-1, NULL);
                return -1;
        }

        *path = (char*)malloc(sizeof(char)*(HTTP_MAX_PATH+1));
        if (*path == NULL) {
                perror("malloc");
                NFREE(mth_str);
                next_header(-1, NULL);
                return -1;
        }

        /* FIXME works on 'localhost' but not 'localhost/' */
        if (sscanf(line, "%16s %128s HTTP/%*s", mth_str, *path) < 2) {
                WLOG("Cannot scan first header line");
                perror("sscanf");
                NFREE(mth_str);
                goto MALFORMED;
        }

        *mth = http_mth(mth_str);
        NFREE(mth_str);

        if (*mth != HTTP_M_POST) {
                /* no request body */
                next_header(-1, NULL);
                return 0;
        }

        /* body length */
        *len = -1;
        while ((line = next_header(sock, &eoh)) != NULL) {
                if (eoh) {
                        WLOG("Got end of headers without Content-Length");
                        goto MALFORMED;
                }

                if (strlen(line) < HTTP_HEADER_CL_LEN) {
                        continue;
                }

                if (strncasecmp(line,
                                HTTP_HEADER_CL, HTTP_HEADER_CL_LEN) == 0) {

                        if (sscanf(line, HTTP_HEADER_CL " %24d", len) < 1) {
                                WLOG("Cannot parse Content-Length");
                                perror("sscanf");
                                goto MALFORMED;
                        }

                        if (*len < 0) {
                                WLOG("Got a negative Content-Length");
                                goto MALFORMED;
                        }
                        break;
                }

                NFREE(line);
        }
        if (*len == -1) {
                WLOG("Didn't got a Content-Length");
                goto MALFORMED;
        }
        while (!eoh) {
                line = next_header(sock, &eoh);

                if (line == NULL) {
                        goto MALFORMED;
                }
        }

        /* request body */
        *body = (char*)malloc(sizeof(char)*(*len));
        if (*body == NULL) {
                goto MALFORMED;
        }

        memcpy(body, line, eoh);
        NFREE(line);

        readlen = 0;
        body_len = eoh;
        while (body_len < *len
                && (readlen = recv(sock, (*body)+body_len,
                                        (*len)-body_len, 0)) > 0) {
                body_len += readlen;
        }
        if (body_len < *len) {
                NFREE(*body);
                goto MALFORMED;
        }

        return 0;

MALFORMED:
        NFREE(*path);
        NFREE(line);
        next_header(-1, NULL);
        return -1;
}

