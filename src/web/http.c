#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "http.h"
#include "utils.h"

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
}; /* length: 16 */

const char *get_http_status_phrase(int *code) {
        int badreq_i = -1;
        for (int i=0; i<16; i++) {
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
        hs->size = 0;
        hs->headers = (struct http_header**)malloc(
                                sizeof(struct http_header*)*HTTP_MAX_HEADERS);

        if (hs->headers == NULL) {
                perror("malloc");
                return -1;
        }

        for (int i=0; i<HTTP_MAX_HEADERS; i++) {
                hs->headers[i] = NULL;
        }

        return 0;
}

int http_add_header(struct http_headers *hs, char *name, char *value,
                                int overr) {
        struct http_header *h;
        int namelen,
            valuelen;

        if (hs == NULL || hs->headers == NULL || hs->size < 0
                        || name == NULL || value == NULL) {
                return -1;
        }

        if (hs->size >= HTTP_MAX_HEADERS) {
                return -1;
        }

        namelen = strlen(name);
        valuelen = strlen(value);

        for (int i=0, s=hs->size; i<s; i++) {
                if (strcasecmp(hs->headers[i]->name, name) == 0) {
                        if (!overr) {
                                return -2;
                        }
                        if (memcpy(hs->headers[i]->value,
                                        value, valuelen) == NULL) {
                                perror("memcpy");
                                return -1;
                        }
                        return 0;
                }
        }

        h = (struct http_header*)malloc(sizeof(struct http_header));

        if (h == NULL) {
                perror("malloc");
                return -1;
        }

        h->name = (char *)malloc(sizeof(char)*(namelen+1));
        if (h->name == NULL) {
                perror("malloc");
                NFREE(h);
                return -1;
        }

        h->value = (char *)malloc(sizeof(char)*(valuelen+1));
        if (h->value == NULL) {
                perror("malloc");
                NFREE(h->name);
                NFREE(h);
                return -1;
        }

        if (memcpy(h->name, name, namelen+1) == NULL
                        || memcpy(h->value, value, valuelen+1) == NULL) {
                perror("memcpy");
                NFREE(h->name);
                NFREE(h->value);
                NFREE(h);
                return -1;
        }

        hs->headers[hs->size++] = h;
        return 0;
}

/* return the length of a string representation of this header, including CRLF,
 * but excluding \0.
 */
int http_header_size(struct http_header *h) {
        if (h == NULL || h->name == NULL || h->value == NULL) {
                return -1;
        }
        /* <name>: <value>\r\n */
        return strlen(h->name) + 2 + strlen(h->value) + 2;
}

int http_headers_size(struct http_headers *hs) {
        int size = 0;

        if (hs == NULL || hs->headers == NULL) {
                return -1;
        }

        for (int i=0, s=hs->size, hhs; i<s; i++) {
                if (hs->headers[i] == NULL) {
                        continue;
                }
                hhs = http_header_size(hs->headers[i]);
                if (hhs > 0) {
                        size += hhs;
                }
        }

        /* with \0 */
        return size;
}

char *http_header_string(struct http_header *h) {
        int len = http_header_size(h);
        char *repr;

        if (len < 0) {
                return NULL;
        }

        repr = (char*)malloc(sizeof(char)*(len+1));

        if (repr == NULL) {
                perror("malloc");
                return NULL;
        }

        snprintf(repr, len+1, "%s: %s\r\n", h->name, h->value);
        return repr;
}

char *http_headers_string(struct http_headers *hs) {
        int len = http_headers_size(hs);
        char *repr;

        if (len < 0) {
                return NULL;
        }

        repr = (char*)malloc(sizeof(char)*(len+1));
        if (repr == NULL) {
                perror("malloc");
                return NULL;
        }
        repr[0] = '\0';

        for (int i=0; i<hs->size; i++) {
                char *s = http_header_string(hs->headers[i]);
                if (s == NULL) {
                        continue;
                }
                strncat(repr, s, strlen(s));
                NFREE(s);
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

        for (int i=0, s=hs->size; i<s; i++) {
                NFREE(hs->headers[i]->name);
                NFREE(hs->headers[i]->value);
                NFREE(hs->headers[i]);
        }
        NFREE(hs->headers);
        free(hs);
        return 0;
}
