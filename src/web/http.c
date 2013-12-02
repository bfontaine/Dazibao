#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "http.h"
#include "utils.h"
#include "webutils.h"

static const struct http_status http_statuses[] = {
        { HTTP_S_OK            , "OK"                    },
        { HTTP_S_CREATED       , "Created"               },
        { HTTP_S_NO_CT         , "No Content"            },
        { HTTP_S_RESET_CT      , "Reset Content"         },

        { HTTP_S_MLTPL_CHOICES , "Multiple Choices"      },
        { HTTP_S_MOVED         , "Moved Permanently"     },
        { HTTP_S_FOUND         , "Not Found"             },

        { HTTP_S_BADREQ        , "Bad Request"           },
        { HTTP_S_FORBIDDEN     , "Forbidden"             },
        { HTTP_S_NOTFOUND      , "Not Found"             },
        { HTTP_S_NOTALLOWED    , "Not Allowed"           },
        { HTTP_S_CONFLICT      , "Conflict"              },
        { HTTP_S_LENGTHREQD    , "Length Required"       },
        { HTTP_S_URITOOLONG    , "Request-URI Too Long"  },

        { HTTP_S_ERR           , "Internal Server Error" },
        { HTTP_S_NOTIMPL       , "Not Implemented"       },
        { HTTP_UNSUPP_VER      , "Version Not Supported" }
};
static const int http_statuses_len =
                sizeof(http_statuses)/sizeof(struct http_status);

static const char *headers_strs[] = {
        /* HTTP_H_CONTENT_TYPE   0 */
        "Content-Type",
        /* HTTP_H_CONTENT_LENGTH 1 */
        "Content-Length",
        /* HTTP_H_HOST           2 */
        "Host",
        /* HTTP_H_UA             3 */
        "User-Agent",
        /* HTTP_H_ALLOW          4 */
        "Allow",
        /* HTTP_H_DATE           5 */
        "Date",
        /* HTTP_H_SERVER         6 */
        "Server",
        /* HTTP_H_POWEREDBY      7 */
        "X-Powered-By",
        /* HTTP_H_ACCEPT         8 */
        "Accept"
};

char is_crlf(char *s, int c, int len) {
        return s != NULL && c < len-1 && s[c] == CR && s[c+1] == LF;
}

int get_http_header_code(char *str) {
        if (str == NULL) {
                return -2;
        }
        for (int i=0; i<HTTP_MAX_HEADERS && headers_strs[i] != NULL; i++) {
                if (strcasecmp(headers_strs[i], str) == 0) {
                    return i;
                }
        }
        return -1;
}

char *get_http_header_str(int code) {
        if (code < 0 || code >= HTTP_MAX_HEADERS) {
                return NULL;
        }
        return strdup(headers_strs[code]);
}

const char *get_http_status_phrase(int *code) {
        int badreq_i = -1;
        for (int i=0; i<http_statuses_len; i++) {
                if (http_statuses[i].code == *code) {
                        return http_statuses[i].phrase;
                }
                if (http_statuses[i].code == HTTP_S_BADREQ) {
                        badreq_i = i;
                }
        }

        *code = HTTP_S_BADREQ;
        return badreq_i >= 0 ? http_statuses[badreq_i].phrase : NULL;
}

int http_mth(char *s) {
        if (strcasecmp(s, "GET") == 0) {
                return HTTP_M_GET;
        }
        if (strcasecmp(s, "POST") == 0) {
                return HTTP_M_POST;
        }
        return HTTP_M_UNSUPPORTED;
}

int http_init_headers(struct http_headers *hs) {
        if (hs == NULL) {
                return -1;
        }
        hs->headers = (char**)malloc(sizeof(char*)*HTTP_MAX_HEADERS);
        if (hs->headers == NULL) {
                perror("malloc");
                return -1;
        }

        for (int i=0; i<HTTP_MAX_HEADERS; i++) {
                hs->headers[i] = NULL;
        }

        return 0;
}

int http_add_header(struct http_headers *hs, int code, const char *value,
                                char overr) {
        if (hs == NULL || hs->headers == NULL || code < 0
                        || code > HTTP_MAX_HEADERS || value == NULL) {
                return -1;
        }

        if (hs->headers[code] != NULL) {
                if (!overr) {
                        WLOGDEBUG("Cannot override header %d with '%s'",
                                        code, value);
                        return -1;
                }
                WLOGDEBUG("Overriding header %d (%s) with '%s'",
                                code, hs->headers[code], value);
                NFREE(hs->headers[code]);
        }

        hs->headers[code] = strdup(value);
        return 0;
}

/* return the length of a string representation of this header, including CRLF,
 * but excluding \0.
 */
int http_header_size(int code, char *value) {
        char *name;
        int len = 0;
        if (code < 0 || code > HTTP_MAX_HEADERS || value == NULL) {
                return -1;
        }
        if ((name = get_http_header_str(code)) == NULL) {
                free(name);
                return -1;
        }
        /* <name> COLON SPACE <value> CR LF */
        len = strlen(name) + 2 + strlen(value) + 2;
        free(name);
        return len;
}

int http_headers_size(struct http_headers *hs) {
        int size = 0;

        if (hs == NULL || hs->headers == NULL) {
                return -1;
        }

        for (int i=0, hhs; i<HTTP_MAX_HEADERS; i++) {
                if (hs->headers[i] == NULL) {
                        continue;
                }
                hhs = http_header_size(i, hs->headers[i]);
                if (hhs > 0) {
                        size += hhs;
                }
        }

        /* without \0 */
        return size;
}

char *http_header_string(int code, char *value) {
        char *name, *repr;
        int len;

        if (code < 0 || code > HTTP_MAX_HEADERS || value == NULL) {
                return NULL;
        }

        name = get_http_header_str(code);
        if (name == NULL) {
                free(name);
                return NULL;
        }

        len = strlen(name) + 2 + strlen(value) + 2;
        repr = (char*)malloc(sizeof(char)*(len+1));

        if (repr == NULL) {
                perror("malloc");
                free(name);
                return NULL;
        }

        snprintf(repr, len+1, "%s: %s\r\n", name, value);
        free(name);
        return repr;
}

char *http_headers_string(struct http_headers *hs) {
        char *reprs[HTTP_MAX_HEADERS],
             *repr;
        int lens[HTTP_MAX_HEADERS];
        int len = 0;

        if (hs == NULL || hs->headers == NULL) {
                return NULL;
        }

        for (int i=0; i<HTTP_MAX_HEADERS; i++) {
                if (hs->headers[i] == NULL) {
                        reprs[i] = NULL;
                        continue;
                }
                reprs[i] = http_header_string(i, hs->headers[i]);
                if (reprs[i] != NULL) {
                        lens[i] = strlen(reprs[i]);
                        len += lens[i];
                }
        }

        repr = (char*)malloc(sizeof(char)*(len+1));
        if (repr == NULL) {
                perror("malloc");
                return NULL;
        }
        repr[0] = '\0';

        for (int i=0; i<HTTP_MAX_HEADERS; i++) {
                if (reprs[i] == NULL) {
                        continue;
                }
                strncat(repr, reprs[i], lens[i]);
                NFREE(reprs[i]);
        }

        return repr;
}

int destroy_http_headers(struct http_headers *hs) {
        if (hs == NULL) {
                return 0;
        }

        if (hs->headers == NULL) {
                NFREE(hs);
                return 0;
        }

        for (int i=0; i<HTTP_MAX_HEADERS; i++) {
                if (hs->headers[i] == NULL) {
                        continue;
                }
                NFREE(hs->headers[i]);
        }
        NFREE(hs->headers);
        free(hs);
        return 0;
}
