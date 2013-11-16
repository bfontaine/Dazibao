#include "webutils.h"
#include <unistd.h>

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
