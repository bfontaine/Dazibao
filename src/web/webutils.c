#include <unistd.h>
#include <string.h>
#include "webutils.h"
#include "tlv.h"

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
