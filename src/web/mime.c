#include <stdio.h>
#include <string.h>
#include "webutils.h"
#include "mime.h"

const char *get_mime_type(const char *path) {
        char *dot;
        if (path == NULL) {
                return NULL;
        }

        LOGDEBUG("Looking for a mime type for '%s'", path);

        if ((dot = strrchr(path, '.')) == NULL) {
                LOGDEBUG("No dot = no extension = no mime type");
                return NULL;
        }
        dot++;
        LOGDEBUG("extension: %s", dot);

        for (unsigned int i=0; i<MIME_TYPES_COUNT; i++) {
                if (strcasecmp(mime_types_ext[i][0], dot) == 0) {
                        LOGDEBUG("Using '%s'", mime_types_ext[i][1]);
                        return mime_types_ext[i][1];
                }
        }
        return NULL;
}
