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
#define HTML_TLV_FMT(F)   "<li class=\"tlv\">\n" \
                           "<div class=\"metadata\">" \
                             "<span class=\"type\">%d</span>" \
                             " <span class=\"length\">%u</span>" \
                           "</div>\n" \
                           "<div class=\"value\">" F "</div>\n" \
                         "</li>"
#define HTML_DZ_BOTTOM  "</ol>\n" \
                      "</body>\n" \
                    "</html>"

/* hoping that it'll be optimized by GCC */
#define HTML_DZ_TOP_LEN strlen(HTML_DZ_TOP)
#define HTML_DZ_BOTTOM_LEN strlen(HTML_DZ_BOTTOM)

/* Use HTML_TLV_FMT(F) to declare the format of a TLV HTML, e.g.:
 *      char fmt[] = HTML_TLV_FMT("<p>%s</p>");
 */

#define JPEG_EXT ".jpg"
#define PNG_EXT  ".png"
#define DEFAULT_EXT ""

/**
 * Make a NULL-terminated HTML representation of a TLV, assuming its value is
 * in plain text.
 **/
int text_tlv2html(tlv_t *t, int type, unsigned int len, char *html);

/**
 * Make a NULL-terminated HTML representation of an image TLV (png or jpeg).
 * 'ext' should be either JPEG_EXT or PNG_EXT.
 **/
int img_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html, const char *ext);

/**
 * Make a NULL-terminated HTML representation of a TLV.
 **/
int tlv2html(dz_t dz, tlv_t *t, off_t off, char **html);

/**
 * Make a NULL-terminated HTML representation of a dazibao.
 **/
int dz2html(dz_t dz, char **html);

#endif
