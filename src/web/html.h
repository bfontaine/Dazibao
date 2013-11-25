#ifndef _HTML_H
#define _HTML_H 1

#include "dazibao.h"
#include "tlv.h"
#include "string.h"

#define HTML_DZ_VERSION "v0.0.1-alpha"

#define HTML_DZ_TOP "<!doctype html>\n" \
                    "<html lang=\"en\" dir=\"ltr\">\n" \
                      "<head>\n" \
                        "<meta charset=\"utf-8\" />\n" \
                        "<meta name=\"language\" content=\"fr\" />\n" \
                        "<title>Dazibao " HTML_DZ_VERSION "</title>\n" \
                      "</head>\n" \
                      "<body>\n" \
                        "<ol class=\"tlvs\">\n"

#define HTML_DZ_BOTTOM  "</ol>\n" \
                      "</body>\n" \
                    "</html>"

/* hoping that it'll be optimized by GCC */
#define HTML_DZ_TOP_LEN strlen(HTML_DZ_TOP)
#define HTML_DZ_BOTTOM_LEN strlen(HTML_DZ_BOTTOM)


/**
 * Make an HTML representation of a TLV, NULL-terminated.
 **/
int tlv2html(dz_t dz, tlv_t t, off_t off, char **html);

/**
 * Make an HTML representation of a dazibao, NULL-terminated.
 **/
int dz2html(dz_t dz, char **html);

#endif
