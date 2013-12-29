#include <stdio.h>
#include <string.h>
#include "webutils.h"
#include "utils.h"
#include "logging.h"
#include "mime.h"

/** @file */

const char *get_mime_type_from_path(const char *path) {
        const char *ext;
        if (path == NULL) {
                return NULL;
        }

        LOGDEBUG("Looking for a mime type for '%s'", path);

        if ((ext = get_ext(path)) == NULL) {
                LOGDEBUG("no extension = no mime type");
                return NULL;
        }
        LOGDEBUG("extension: %s", ext);

        for (unsigned int i=0; i<MIME_TYPES_COUNT; i++) {
                if (strcasecmp(tlv_mime_types[i].ext, ext) == 0) {
                        LOGDEBUG("Using '%s'", tlv_mime_types[i].mime);
                        return tlv_mime_types[i].mime;
                }
        }
        return NULL;
}

const char *get_mime_type_from_tlv(unsigned char type) {
        LOGDEBUG("Looking for a mime type for TLV '%d'", (int)type);

        for (unsigned int i=0; i<MIME_TYPES_COUNT; i++) {
                if (type == tlv_mime_types[i].type) {
                        LOGDEBUG("Using '%s'", tlv_mime_types[i].mime);
                        return tlv_mime_types[i].mime;
                }
        }
        return NULL;
}
