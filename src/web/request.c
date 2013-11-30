#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "request.h"
#include "http.h"
#include "webutils.h"
#include "utils.h"

struct http_request *create_http_request(void) {
        struct http_request *req;

        req = (struct http_request*)malloc(sizeof(struct http_request));

        if (req == NULL) {
                return NULL;
        }
        req->method = req->body_len = -1;
        req->path = req->body = NULL;
        req->headers = \
              (struct http_headers*)malloc(sizeof(struct http_headers));
        http_init_headers(req->headers);
        return req;
}

int reset_http_request(struct http_request *req) {
        int st;
        if (req == NULL) {
                return -1;
        }
        req->method = -1;
        NFREE(req->path);
        NFREE(req->body);
        st = destroy_http_headers(req->headers);
        req->headers = NULL;

        return st;
}

int destroy_http_request(struct http_request *req) {
        int st = reset_http_request(req);
        free(req);
        return st;
}

int parse_request(int sock, struct http_request *req) {
        char *line,
             *mth_str,
             status_fmt[] = "%" HTTP_MAX_MTH_LENGTH_S "s " \
                            "%" HTTP_MAX_PATH_S "s HTTP/%*s";

        int eoh = 0,
            readlen = 0,
            body_len = 0,
            status = 0;

        if (req == NULL) {
                return -1;
        }

        line = next_header(sock, &eoh); /* First line */
        if (line == NULL || eoh) {
                WLOGWARN("Cannot get the first header line (eoh=%d)", eoh);
                next_header(-1, NULL);
                return HTTP_S_BADREQ;
        }

        mth_str = (char*)malloc(sizeof(char)*(HTTP_MAX_MTH_LENGTH+1));
        if (mth_str == NULL) {
                perror("malloc");
                next_header(-1, NULL);
                return HTTP_S_BADREQ;
        }

        if (req->path == NULL) {
                req->path = (char*)malloc(sizeof(char)*(HTTP_MAX_PATH+1));
        }
        if (req->path == NULL) {
                perror("malloc");
                NFREE(mth_str);
                next_header(-1, NULL);
                return HTTP_S_BADREQ;
        }

        if (sscanf(line, status_fmt, mth_str, req->path) < 2) {
                WLOGWARN("Cannot scan first header line");
                perror("sscanf");
                NFREE(mth_str);
                goto MALFORMED;
        }

        req->method = http_mth(mth_str);
        NFREE(mth_str);

        if (req->method == HTTP_M_UNSUPPORTED) {
            status = HTTP_S_NOTIMPL;
            goto EOPARSING;
        }

        if (req->method != HTTP_M_POST) {
                /* no request body */
                goto EOPARSING;
        }

        if (req->headers != NULL) {
                destroy_http_headers(req->headers);
                req->headers = NULL;
        }

        if (req->headers == NULL || !http_init_headers(req->headers)) {
                WLOGERROR("Couldn't allocate memory for headers.");
                perror("malloc");
                goto MALFORMED;
        }

        /* Other headers */
        while ((line = next_header(sock, &eoh)) != NULL && !eoh) {
                if (parse_header(line, req->headers) != 0) {
                        WLOGDEBUG("Couldn't parse header <%s>", line);
                }

                NFREE(line);
        }

        while (!eoh) {
                free(line);
                line = next_header(sock, &eoh);

                if (line == NULL) {
                        goto MALFORMED;
                }
        }

        if (req->headers->headers[HTTP_H_CONTENT_LENGTH] == NULL) {
                WLOGERROR("Got no content length header");
                status = HTTP_S_LENGTHREQD;
                goto EOPARSING;
        }
        req->body_len = strtol(req->headers->headers[HTTP_H_CONTENT_LENGTH],
                                NULL, 10);

        if (STRTOL_ERR(req->body_len) || req->body_len <= 0) {
                WLOGERROR("Got malformed content length header");
                status = HTTP_S_LENGTHREQD;
                goto EOPARSING;
        }

        /* request body */
        req->body = (char*)malloc(sizeof(char)*(req->body_len));
        if (req->body == NULL) {
                WLOGERROR("Cannot alloc for the request body");
                perror("malloc");
                goto MALFORMED;
        }

        memcpy(req->body, line, eoh);
        NFREE(line);

        readlen = 0;
        body_len = eoh;
        while (body_len < req->body_len
                && (readlen = read(sock, req->body + body_len,
                                        req->body_len - body_len)) > 0) {
                body_len += readlen;
        }
        if (body_len < req->body_len) {
                goto MALFORMED;
        }

        return 0;

MALFORMED:
        status = HTTP_S_BADREQ;
        /*destroy_http_request(req);*/
EOPARSING:
        NFREE(line);
        next_header(-1, NULL);
        return status;
}

char *next_header(int sock, int *eoh) {
        static char rest[BUFFLEN];
        static int restlen = 0;

        char buff[BUFFLEN];
        char *line;
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
                len = read(sock, buff, BUFFLEN);

                if (len < 0) {
                        perror("read");
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
                len = read(sock, buff, BUFFLEN);
                if (len <= 0) {
                        if (len < 0) {
                                perror("read");
                        }
                        restlen = 0;
                        goto RETURN_LINE;
                }

                line = safe_realloc(line, i+BUFFLEN);

                if (line == NULL) {
                        perror("realloc");
                        NFREE(line);
                        restlen = 0;
                        return NULL;
                }

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

int parse_header(char *line, struct http_headers *hs) {
        int code, idx, idx2;
        char *name, *value;

        if (line == NULL || hs == NULL || hs->headers == NULL) {
                return -1;
        }

        idx = -1;
        for (int i=0; line[i] != '\0'; i++) {
                if (line[i] == ':') {
                        idx = i;
                        break;
                }
        }
        if (idx < 1 || idx >= HTTP_MAX_HEADER_NAME_LENGTH - 1) {
                return -1;
        }

        name = (char*)malloc(sizeof(char)*(idx+1));
        if (name == NULL) {
                return -1;
        }

        memcpy(name, line, idx);
        name[idx++] = '\0';

        code = get_http_header_code(name);
        if (code < 0) {
                NFREE(name);
                return -1;
        }

        value = (char*)malloc(sizeof(char)*HTTP_MAX_HEADER_VALUE_LENGTH);
        if (value == NULL) {
                free(name);
                return -1;
        }

        while (line[idx++] == ' '); /* skip spaces */

        idx2 = 0;
        while (line[idx] != '\0' && idx2 < HTTP_MAX_HEADER_VALUE_LENGTH - 1) {
                value[idx2++] = line[idx++];
        }
        /* TODO check if value ends with CRLF (if so, remove them) */
        value[idx2] = '\0';

        return http_add_header(hs, code, value, 1);
}
