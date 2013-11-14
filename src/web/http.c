#include "http.h"
#include <string.h>

int http_mth(char *s) {
    switch (s[0]) {
        case 'G': return HTTP_M_GET;
        case 'P': return HTTP_M_POST;
        default: return HTTP_M_UNSUPPORTED;
    }
}

