#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "request.h"
#include "logging.h"
#include "http.h"
#include "webutils.h"
#include "utils.h"

/** @file */

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
             status_fmt[] = "%" STR(HTTP_MAX_MTH_LENGTH) "s " \
                            "%" STR(HTTP_MAX_PATH) "s HTTP/%*s";

        int linelen,
            eoh = 0,
            readlen = 0,
            body_len = 0,
            status = 0;

        if (req == NULL) {
                LOGDEBUG("Got a NULL req struct");
                return -1;
        }

        line = next_header(sock, &eoh); /* First line */
        if (line == NULL || eoh) {
                LOGWARN("Cannot get the first header line (eoh=%d)", eoh);
                status = HTTP_S_BADREQ;
                goto EOPARSING;
        }

        linelen = strlen(line);

        mth_str =(char*)malloc(sizeof(char) * (
                                MIN(HTTP_MAX_MTH_LENGTH, linelen)+1));
        if (mth_str == NULL) {
                perror("malloc");
                status = HTTP_S_BADREQ;
                goto EOPARSING;
        }

        if (req->path == NULL) {
                req->path = (char*)malloc(sizeof(char)*(
                                        MIN(HTTP_MAX_PATH, linelen)+1));
        }
        if (req->path == NULL) {
                perror("malloc");
                NFREE(mth_str);
                status = HTTP_S_BADREQ;
                goto EOPARSING;
        }

        if (sscanf(line, status_fmt, mth_str, req->path) < 2) {
                LOGWARN("Cannot scan first header line");
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

        if (req->headers != NULL) {
                destroy_http_headers(req->headers);
                req->headers = NULL;
        }
        req->headers =
                (struct http_headers*)malloc(sizeof(struct http_headers));


        if (req->headers == NULL || http_init_headers(req->headers) != 0) {
                LOGERROR("Couldn't allocate memory for headers.");
                perror("malloc");
                goto MALFORMED;
        }

        /* Other headers */
        NFREE(line);
        while ((line = next_header(sock, &eoh)) != NULL && !eoh) {
                if (parse_header(line, req->headers) != 0) {
                        LOGDEBUG("Couldn't parse header <%s>", line);
                }

                NFREE(line);
        }

        if (req->method != HTTP_M_POST) {
                /* no request body */
                goto EOPARSING;
        }

        while (!eoh) {
                free(line);
                line = next_header(sock, &eoh);

                if (line == NULL) {
                        goto MALFORMED;
                }
        }

        if (req->headers->headers[HTTP_H_CONTENT_LENGTH] == NULL) {
                LOGERROR("Got no content length header");
                status = HTTP_S_LENGTHREQD;
                goto EOPARSING;
        }
        req->body_len =
                str2dec_positive(req->headers->headers[HTTP_H_CONTENT_LENGTH]);

        if (!IN_RANGE(req->body_len, 0, INT_MAX)) {
                LOGERROR("Got no or malformed Content-Length header");
                status = HTTP_S_LENGTHREQD;
                goto EOPARSING;
        }

        /* request body */
        req->body = (char*)malloc(sizeof(char)*(MAX(req->body_len, eoh)));
        if (req->body == NULL) {
                LOGERROR("Cannot alloc for the request body");
                perror("malloc");
                goto MALFORMED;
        }

        LOGTRACE("adding line '%.*s' into the beginning of the body (eoh=%d)",
                        eoh, line, eoh);
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

        next_header(-1, NULL);
        return 0;

MALFORMED:
        status = HTTP_S_BADREQ;
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

        NFREE(line);
        restlen = 0;
        return NULL;

RETURN_LINE:
        if (i == 0) {
                if (memcpy(line, buff, len) == NULL) {
                        perror("memcpy");
                }

                if (len > 2 && line[0] == CR && line[1] == LF) {
                        len -= 2;
                        /* shift the line two chars to the right */
                        memmove(line, line+2, len);
                }

                *eoh = len;
                return line;
        }
        line[i] = '\0';
        LOGTRACE("[header line] %s", line);
        return line;
}

int parse_header(char *line, struct http_headers *hs) {
        int code, idx, idx2, st;
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
        NFREE(name);
        if (code < 0) {
                return -1;
        }

        value = (char*)malloc(sizeof(char)*HTTP_MAX_HEADER_VALUE_LENGTH);
        if (value == NULL) {
                return -1;
        }

        while (line[idx] == ' ') { /* skip spaces */
                idx++;
        }

        idx2 = 0;
        while (line[idx] != '\0' && idx2 < HTTP_MAX_HEADER_VALUE_LENGTH - 1) {
                value[idx2++] = line[idx++];
        }
        value[idx2] = '\0';

        st = http_add_header(hs, code, value, 1);
        free(value);
        return st;
}

char *get_request_boundary(struct http_request *req) {
        int len, blen;
        char *ct,
             *b;
        const char boundary[] = "boundary=";

        if (req == NULL || req->headers == NULL) {
                return NULL;
        }

        ct = req->headers->headers[HTTP_H_CONTENT_TYPE];

        if (ct == NULL || (b = strstr(ct, boundary)) == NULL) {
                return NULL;
        }

        len  = strlen(b); /* length of "boundary=..." */
        blen = strlen(boundary); /* length of "boundary=" */

        if (len == blen) {
                /* if the line ends with "... boundary=", i.e. the boundary is
                 * empty */
                return NULL;
        }

        /* shift 'strlen("boundary=")' to the right to keep only the boundary
         * string. */
        return b + blen;
}
