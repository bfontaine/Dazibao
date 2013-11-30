#include "response.h"

struct http_response *create_http_response(void) {
        struct http_response *resp;

        resp = (struct http_response*)malloc(sizeof(struct http_response));

        if (resp == NULL) {
                return NULL;
        }
        resp->status = resp->body_len = -1;
        resp->body = NULL;
        resp->headers = \
              (struct http_headers*)malloc(sizeof(struct http_headers));
        http_init_headers(resp->headers);
        return resp;
}

int destroy_http_response(struct http_response *resp) {
        int st;
        if (resp == NULL) {
                return -1;
        }
        NFREE(resp->body);
        st = destroy_http_headers(resp->headers);
        resp->headers = NULL;
        NFREE(resp);
        return st;
}
