#include "http.h"
#include "utils.h"
#include <string.h>

static struct http_statut http_statuts[] = {
        { HTTP_S_OK            , "OK"                    },
        { HTTP_S_CREATED       , "Created"               },
        { HTTP_S_NO_CT         , "No Content"            },
        { HTTP_S_RESET_CT      , "Reset Content"         },

        { HTTP_S_MLTPL_CHOICES , "Multiple Choices"      },
        { HTTP_S_MOVED         , "Moved Permanently"     },
        { HTTP_S_FOUND         , "Not Found"             },

        { HTTP_S_BADREQ        , "Bad Request"           },
        { HTTP_S_FORBIDDEN     , "Forbidden"             },
        { HTTP_S_NOTFOUND      , "Not Found"             },
        { HTTP_S_CONFLICT      , "Conflict"              },
        { HTTP_S_LENGTHREQD    , "Length Required"       },
        { HTTP_S_URITOOLONG    , "Request-URI Too Long"  },

        { HTTP_S_ERR           , "Internal Server Error" },
        { HTTP_S_NOTIMPL       , "Not Implemented"       },
        { HTTP_UNSUPP_VER      , "Version Not Supported" }
}; /* length: 16 */

const char *get_http_status_phrase(int *code) {
        int badreq_i = -1;
        for (int i=0; i<16; i++) {
                if (http_statuts[i].code == *code) {
                        return http_statuts[i].phrase;
                }
                if (http_statuts[i].code == HTTP_S_BADREQ) {
                        badreq_i = i;
                }
        }

        *code = HTTP_S_BADREQ;
        return badreq_i >= 0 ? http_statuts[badreq_i].phrase : NULL;
}

int http_mth(char *s) {
        int len = strlen(s);
        if (strncasecmp(s, "GET", MIN(len, 3)) == 0) {
                return HTTP_M_GET;
        }
        if (strncasecmp(s, "POST", MIN(len, 4)) == 0) {
                return HTTP_M_POST;
        }
        return HTTP_M_UNSUPPORTED;
}

