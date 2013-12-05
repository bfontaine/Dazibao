#ifndef _HTML_H
#define _HTML_H 1

/** @file
 * Utilities to generate HTML code
 **/

#include "dazibao.h"
#include "tlv.h"
#include "string.h"

/** maximum length of a Dazibao filename in an HTML page */
#define HTML_DZ_MAX_NAME_LENGTH 64
/** maximum HTML length of a TLV */
#define HTML_TLV_SIZE 1024

/** Format used for the HTML top of a dazibao */
#define HTML_DZ_TOP_FMT \
                  "<!doctype html>\n" \
                    "<html lang=\"en\" dir=\"ltr\">\n" \
                      "<head>\n" \
                        "<meta charset=\"utf-8\" />\n" \
                        "<meta name=\"language\" content=\"fr\" />\n" \
                        "<link rel=\"stylesheet\" href=\"/dz.css\">" \
                        "<title>Dazibao - %s</title>\n" \
                      "</head>\n" \
                      "<body>\n" \
                        "<ol class=\"tlvs\">\n"
/**
 * Format used for the HTML of a TLV.
 * @param F the format of the value of the TLV (may be an empty string)
 **/
#define HTML_TLV_FMT(F)   "<li class=\"tlv\" data-offset=\"%lu\">\n" \
                           "<div class=\"metadata\">" \
                             "<span class=\"type\">" \
                               "%s (<span class=\"id\">%d</span>)" \
                             "</span>" \
                             " <span class=\"length\">%u</span>" \
                           "</div>\n" \
                           "<div class=\"value\">" F "</div>\n" \
                         "</li>"
/** Format used for the HTML bottom of a dazibao */
#define HTML_DZ_BOTTOM  "</ol>\n" \
                      "<script src=\"/dz.js\"></script>\n" \
                      "</body>\n" \
                    "</html>"

/* Use HTML_TLV_FMT(F) to declare the format of a TLV HTML, e.g.:
 *      char fmt[] = HTML_TLV_FMT("<p>%s</p>");
 */

/**
 * Make a NULL-terminated HTML representation of a TLV, assuming its value is
 * in plain text.
 **/
int text_tlv2html(tlv_t *t, int type, unsigned int len, off_t off, char *html);

/**
 * Make a NULL-terminated HTML representation of an image TLV (png or jpeg).
 * 'ext' should be either JPEG_EXT or PNG_EXT.
 **/
int img_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html, const char *ext);

/**
 * Return a NULL-terminated HTML representation of a dated TLV.
 **/
int dated_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html);

/**
 * Return a NULL-terminated HTML representation of a compound TLV.
 **/
int compound_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html);
/**
 * Make a NULL-terminated HTML representation of a PAD1 or PADN.
 **/
int empty_pad_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html);

/**
 * Make a NULL-terminated HTML representation of a TLV.
 **/
int tlv2html(dz_t dz, tlv_t *t, off_t off, char **html);

/**
 * Make a NULL-terminated HTML representation of a dazibao.
 **/
int dz2html(dz_t dz, char **html);

#endif
