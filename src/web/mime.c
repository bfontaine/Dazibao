#include <string.h>
#include "mime.h"

const char *get_mime_type(const char *path) {
        char *dot;
        if (path == NULL) {
                return NULL;
        }
        if ((dot = strrchr(path, '.')) == NULL) {
                return NULL;
        }
        dot++;

        for (unsigned int i=0; i<MIME_TYPES_COUNT; i++) {
                if (strcasecmp(mime_types_ext[i][0], dot) == 0) {
                        return mime_types_ext[i][1];
                }
        }
        return NULL;
}
