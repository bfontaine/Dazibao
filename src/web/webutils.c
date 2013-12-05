#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "webutils.h"
#include "tlv.h"
#include "http.h"

/** @file */

int get_image_tlv_type(const char *path) {
        char *dot;
        if (path == NULL) {
                return -1;
        }
        if ((dot = strrchr(path, '.')) == NULL) {
                return -1;
        }
        if (strcasecmp(dot, PNG_EXT) == 0) {
                return TLV_PNG;
        }
        if (strcasecmp(dot, JPEG_EXT) == 0) {
                return TLV_JPEG;
        }
        return -1;
}

/** maximum length of a date string */
#define MAX_DATE_LENGTH 64

char *gmtdate(time_t secs) {
        char *s,
             *nl;

        if (secs == -2) {
                time(&secs);
        }
        if (secs < 0) {
                return NULL;
        }

        s = (char*)malloc(sizeof(char)*MAX_DATE_LENGTH);
        if (s == NULL) {
                return NULL;
        }

        if (strftime(s, MAX_DATE_LENGTH, HTTP_DATE_FMT, gmtime(&secs)) == 0) {
                free(s);
                return NULL;
        }

        nl = strchr(s, '\n');
        if (nl != NULL) {
                *nl = '\0';
        }

        return s;
}
