#ifndef _WEBUTILS_H
#define _WEBUTILS_H 1

#define WLOG(fmt, ...) { \
        printf("[daziweb:%10s:%15s:%03d] ", __FILE__, __func__, __LINE__); \
        printf(fmt, ##__VA_ARGS__); \
        puts(""); }

#endif
