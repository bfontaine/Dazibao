#include <stdlib.h>
#include <stdio.h>
#include "response.h"
#include "logging.h"
#include "utils.h"

/** @file */

struct http_response *create_http_response(void) {
        struct http_response *resp;

        resp = (struct http_response*)malloc(sizeof(struct http_response));

        if (resp == NULL) {
                perror("malloc");
                return NULL;
        }
        resp->status = resp->body_len = -1;
        resp->body = (char**)malloc(sizeof(char*));

        if (resp->body == NULL) {
                perror("malloc");
                free(resp);
                return NULL;
        }
        *resp->body = NULL;

        resp->headers = \
              (struct http_headers*)malloc(sizeof(struct http_headers));

        if (resp->headers == NULL) {
                perror("malloc");
                NFREE(resp->body);
                free(resp);
                return NULL;
        }

        http_init_headers(resp->headers);
        return resp;
}

int destroy_http_response(struct http_response *resp) {
        int st;
        if (resp == NULL) {
                return -1;
        }
        if (resp->body != NULL) {
                NFREE(*resp->body);
                NFREE(resp->body);
        }
        st = destroy_http_headers(resp->headers);
        resp->headers = NULL;
        free(resp);
        return st;
}
