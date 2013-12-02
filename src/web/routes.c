#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "routes.h"
#include "webutils.h"
#include "routing.h"
#include "dazibao.h"
#include "tlv.h"
#include "http.h"
#include "html.h"

int route_get_index(dz_t dz, struct http_request req,
                        struct http_response *resp) {

        if (strcmp(req.path, "/index.html") != 0 || dz <= 0
                        || req.method != HTTP_M_GET) {
                WLOGERROR("get_index - got wrong path '%s' and/or wrong " \
                                "dz=%d and/or wrong method %d.",
                                req.path, dz, req.method);
                return -1;
        }

        if (dz2html(dz, resp->body) < 0) {
                WLOGERROR("Error while making dazibao's HTML");
                return -1;
        }

        resp->body_len = strlen(*(resp->body));
        resp->status = HTTP_S_OK;

        return 0;
}

int route_get_image_tlv(dz_t dz, struct http_request req,
                        struct http_response *resp) {

        tlv_t *tlv;
        unsigned long off = -1;
        int tlv_type, tlv_real_type;

        if (dz <= 0 || req.method != HTTP_M_GET) {
                WLOGERROR("get_image_tmpv - got wrong dz or method (%d)",
                                req.method);
                return -1;
        }

        tlv = (tlv_t*)malloc(sizeof(tlv_t));
        if (tlv_init(tlv) == -1) {
                WLOGERROR("Cannot allocate memory for a TLV");
                tlv_destroy(tlv);
                return -1;
        }

        tlv_type = get_image_tlv_type(req.path);

        if (tlv_type == -1) {
                WLOGERROR("Cannot get the TLV type from '%s'", req.path);
                tlv_destroy(tlv);
                return -1;
        }

        if (sscanf(req.path, "/tlv/%16lu.%*3s", &off) == 0) {
                WLOGERROR("Cannot parse the request path");
                tlv_destroy(tlv);
                return -1;
        }
        if (off < DAZIBAO_HEADER_SIZE) {
                WLOGERROR("Wrong offset (%lu)", off);
                tlv_destroy(tlv);
                return -1;
        }

        if (dz_tlv_at(&dz, tlv, off) == -1) {
                WLOGERROR("Cannot read TLV at offset %lu", off);
                tlv_destroy(tlv);
                return -1;
        }

        tlv_real_type = tlv_get_type(*tlv);
        if (tlv_real_type != tlv_type) {
                WLOGERROR("Wrong TLV type. Expected %d, got %d",
                                tlv_type, tlv_real_type);
                tlv_destroy(tlv);
                return -1;
        }

        resp->body_len = tlv_get_length(*tlv);
        WLOGDEBUG("TLV is of type %d, with length %d", tlv_type,
                        resp->body_len);

        if (dz_read_tlv(&dz, tlv, off) == -1) {
                WLOGERROR("Cannot read TLV at offset %lu", off);
                tlv_destroy(tlv);
                return -1;
        }

        *resp->body = (char*)malloc(sizeof(char)*resp->body_len);
        if (*resp->body == NULL) {
                perror("malloc");
                WLOGERROR("Cannot alloc memory to store the TLV's value");
                tlv_destroy(tlv);
                return -1;
        }

        memcpy(*resp->body, tlv_get_value_ptr(*tlv), resp->body_len);
        tlv_destroy(tlv);

        resp->status = HTTP_S_OK;

        return 0;
}

int register_routes(void) {
        int st = 0;

        /* Add routes here */
        st |= add_route(HTTP_M_GET, "/index.html", route_get_index);
        st |= add_route(HTTP_M_GET, "/tlv/", route_get_image_tlv);
        return st;
}
