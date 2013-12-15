#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "routes.h"
#include "logging.h"
#include "webutils.h"
#include "routing.h"
#include "mdazibao.h"
#include "tlv.h"
#include "http.h"
#include "html.h"

/** @file */

/** route for GET /index.html */
int route_get_index(dz_t dz, struct http_request req,
                        struct http_response *resp) {

        if (strcmp(req.path, "/index.html") != 0) {
                LOGERROR("get_index - got wrong path '%s' and/or method %d.",
                                req.path, req.method);
                return HTTP_S_BADREQ;
        }

        if (req.method != HTTP_M_HEAD) {
                LOGDEBUG("Generating the HTML of the dazibao...");
                if (dz2html(dz, resp->body) < 0) {
                        LOGERROR("Error while making dazibao's HTML");
                        return HTTP_S_ERR;
                }

                resp->body_len = strlen(*(resp->body));
        } else {
                resp->body_len = -1;
        }

        resp->status = HTTP_S_OK;
        return 0;
}

/** route for GET /tlv/.* */
int route_get_image_tlv(dz_t dz, struct http_request req,
                        struct http_response *resp) {

        tlv_t tlv;
        unsigned long off = -1;
        int tlv_type, tlv_real_type;

        if (tlv_init(&tlv) < 0) {
                LOGERROR("Cannot allocate memory for a TLV");
                tlv_destroy(&tlv);
                return HTTP_S_ERR;
        }

        tlv_type = get_image_tlv_type(req.path);

        if (tlv_type == -1) {
                LOGERROR("Cannot get the TLV type from '%s'", req.path);
                tlv_destroy(&tlv);
                return -1;
        }

        /* We assume that we use only extensions of 3 characters like .png and
         * .jpg (not .jpeg) */
        if (sscanf(req.path, "/tlv/%16lu.%*3s", &off) == 0) {
                LOGERROR("Cannot parse the request path");
                tlv_destroy(&tlv);
                return -1;
        }
        if (off < DAZIBAO_HEADER_SIZE) {
                LOGERROR("Wrong offset (%li < dazibao header size)",
                                (long)off);
                tlv_destroy(&tlv);
                return -1;
        }

        if (dz_tlv_at(&dz, &tlv, off) < 0) {
                LOGERROR("Cannot read TLV at offset %li", (long)off);
                tlv_destroy(&tlv);
                return -1;
        }

        tlv_real_type = tlv_get_type(&tlv);
        if (tlv_real_type != tlv_type) {
                LOGERROR("Wrong TLV type. Expected %d, got %d",
                                tlv_type, tlv_real_type);
                tlv_destroy(&tlv);
                return -1;
        }

        if (req.method != HTTP_M_HEAD) {
                resp->body_len = tlv_get_length(&tlv);
        }
        LOGDEBUG("TLV is of type %d, with length %d", tlv_type,
                        resp->body_len);

        LOGDEBUG("Reading the TLV at offset %lu", (long)off);
        if (dz_read_tlv(&dz, &tlv, off) < 0) {
                LOGERROR("Cannot read TLV at offset %li", (long)off);
                tlv_destroy(&tlv);
                return HTTP_S_ERR;
        }

        if (req.method != HTTP_M_HEAD) {
                *resp->body = (char*)malloc(sizeof(char)*resp->body_len);
                if (*resp->body == NULL) {
                        perror("malloc");
                        LOGERROR("Cannot alloc memory to store the " \
                                        "TLV's value");
                        tlv_destroy(&tlv);
                        return HTTP_S_ERR;
                }

                memcpy(*resp->body, tlv_get_value_ptr(&tlv), resp->body_len);
        }

        tlv_destroy(&tlv);
        resp->status = HTTP_S_OK;
        return 0;
}

/**
 * Route used to delete a TLV
 * @param dz the current dazibao
 * @param req the client request
 * @param resp the response, which will be filled by the function
 * @return 0 on success, -1 on error
 **/
int route_post_rm_tlv(dz_t dz, struct http_request req,
                struct http_response *resp) {

        unsigned long off = -1;

        /* This route returns 204 No Content if the TLV was successfully
           removed. */

        if (sscanf(req.path, "/tlv/delete/%16lu", &off) == 0) {
                LOGERROR("Cannot parse the request path");
                return -1;
        }

        LOGDEBUG("Trying to remove TLV at offset %li in dazibao fd=%d",
                        (long)off, dz.fd);

        if (off < DAZIBAO_HEADER_SIZE) {
                LOGERROR("Wrong offset (%li < dz header size)", (long)off);
                return -1;
        }

        if (dz_rm_tlv(&dz, off) < 0) {
                LOGERROR("Cannot delete TLV at offset %li", (long)off);
                return -1;
        }

        resp->status = HTTP_S_NO_CT;
        resp->body_len = -1;

        return 0;
}

/**
 * Route used to compact a Dazibao
 * @param dz the current dazibao
 * @param req the client request
 * @param resp the response, which will be filled by the function
 * @return 0 on success, -1 on error
 **/
int route_post_compact_dz(dz_t dz, struct http_request req,
                struct http_response *resp) {
        int saved;

        /* This route returns the number of saved bytes if the dazibao was
           successfully compacted. */

        LOGDEBUG("Trying to compact the dazibao...");
        saved = dz_compact(&dz);

        if (saved < 0) {
                LOGWARN("dz_compact gave us an error: %d", saved);
                return HTTP_S_ERR;
        }

        LOGDEBUG("Saved %d bytes", saved);

        /* 16 chars would be sufficient here */
        *resp->body = (char*)malloc(sizeof(char)*16);

        if (snprintf(*resp->body, 16, "%d", saved) > 16) {
                LOGWARN("Cannot fit '%d' into 16 chars", saved);
                *resp->body = strdup("0"); /* easy workaround */
        }

        resp->body_len = strlen(*resp->body);
        resp->status = HTTP_S_OK;

        return 0;
}

int register_routes(void) {
        int st = 0;

        /* Add routes here
         *
         * Remember that routes are matched in the order they're added, so if
         * a route matches '/foo/bar', place it *before* another route which
         * matches '/foo/' or it will never match '/foo/bar*' paths.
         */
        st |= add_route(HTTP_M_GET,  "/index.html",  route_get_index);
        st |= add_route(HTTP_M_POST, "/tlv/delete/", route_post_rm_tlv);
        st |= add_route(HTTP_M_GET,  "/tlv/",        route_get_image_tlv);
        st |= add_route(HTTP_M_POST, "/compact",     route_post_compact_dz);

        return st;
}
