#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "webutils.h"
#include "tlv.h"
#include "http.h"

int _wlog_level =
#ifdef DEBUG
        WLOG_LVL_DEBUG;
#else
        WLOG_LVL_INFO;
#endif

int write_all(int fd, char *buff, int len) {
        int wrote = 0,
            w;

        while (len > 0 && (w = write(fd, buff+wrote, len)) > 0) {
                wrote += w;
                len -= w;
        }

        return wrote;
}

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
