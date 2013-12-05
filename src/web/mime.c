#include <stdio.h>
#include <string.h>
#include "webutils.h"
#include "utils.h"
#include "mime.h"

/** @file */

const char *get_mime_type(const char *path) {
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
                if (strcasecmp(mime_types_ext[i][0], ext) == 0) {
                        LOGDEBUG("Using '%s'", mime_types_ext[i][1]);
                        return mime_types_ext[i][1];
                }
        }
        return NULL;
}
