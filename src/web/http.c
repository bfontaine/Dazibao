#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "http.h"
#include "utils.h"
#include "webutils.h"

static struct http_status http_statuses[] = {
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
        { HTTP_S_CONFLICT      , "Conflict"              },
        { HTTP_S_LENGTHREQD    , "Length Required"       },
        { HTTP_S_URITOOLONG    , "Request-URI Too Long"  },

        { HTTP_S_ERR           , "Internal Server Error" },
        { HTTP_S_NOTIMPL       , "Not Implemented"       },
        { HTTP_UNSUPP_VER      , "Version Not Supported" }
};
static int http_statuses_len =
                sizeof(http_statuses)/sizeof(struct http_status);

static char *headers_strs[] = {
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
        "X-Powered-By"
};

int http_header_code_str(char **str, int *code) {
        int i;

        if ((str == NULL) || (*str == NULL && (code == NULL || *code < 0))) {
                return -2;
        }
        if (*str == NULL) {
                if (*code >= HTTP_MAX_HEADERS) {
                        return -1;
                }
                *str = strdup(headers_strs[*code]);
                return 0;
        }

        for (i=0; i<HTTP_MAX_HEADERS; i++) {
                if (strcasecmp(headers_strs[i], *str) == 0) {
                        *code = i;
                        return 0;
                }
        }

        return -1;
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

int http_add_header(struct http_headers *hs, int code, char *value,
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
        char **name = (char**)malloc(sizeof(char*));
        int len = 0;
        if (code < 0 || code > HTTP_MAX_HEADERS ||
                        value == NULL || name == NULL) {
                return -1;
        }
        *name = NULL;
        if (http_header_code_str(name, &code) != 0) {
                free(name);
                return -1;
        }
        /* <name>: <value>\r\n */
        len = strlen(*name) + 2 + strlen(value) + 2;
        NFREE(*name);
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
        char **name = (char**)malloc(sizeof(char*));
        char *repr;
        int len;

        if (name == NULL || code < 0 || code > HTTP_MAX_HEADERS
                        || value == NULL) {
                return NULL;
        }

        *name = NULL;
        if (http_header_code_str(name, &code) != 0) {
                free(name);
                return NULL;
        }

        len = strlen(*name) + 2 + strlen(value) + 2;
        repr = (char*)malloc(sizeof(char)*(len+1));

        if (repr == NULL) {
                perror("malloc");
                NFREE(*name);
                free(name);
                return NULL;
        }

        snprintf(repr, len+1, "%s: %s\r\n", *name, value);
        NFREE(*name);
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

int http_destroy_headers(struct http_headers *hs) {
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
