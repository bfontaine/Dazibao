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
 * @param t a pointer to the TLV
 * @param type the TLV's type
 * @param len the TLV's length
 * @param off the TLV's offset in the dazibao
 * @param html a preallocated string to store the TLV representation
 * @return 0 on success
 **/
int text_tlv2html(tlv_t *t, int type, unsigned int len, off_t off, char *html);

/**
 * Make a NULL-terminated HTML representation of an image TLV (png or jpeg).
 * 'ext' should be either JPEG_EXT or PNG_EXT.
 * @param t a pointer to the TLV
 * @param type the TLV's type
 * @param len the TLV's length
 * @param off the TLV's offset in the dazibao
 * @param html a preallocated string to store the TLV representation
 * @param ext the extension of the image. This is used to determine its type
 * @return 0 on success
 **/
int img_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html, const char *ext);

/**
 * Return a NULL-terminated HTML representation of a dated TLV.
 * @param t a pointer to the TLV
 * @param type the TLV's type
 * @param len the TLV's length
 * @param off the TLV's offset in the dazibao
 * @param html a preallocated string to store the TLV representation
 * @return 0 on success
 **/
int dated_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html);

/**
 * Return a NULL-terminated HTML representation of a compound TLV.
 * @param t a pointer to the TLV
 * @param type the TLV's type
 * @param len the TLV's length
 * @param off the TLV's offset in the dazibao
 * @param html a preallocated string to store the TLV representation
 * @return 0 on success
 **/
int compound_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html);
/**
 * Make a NULL-terminated HTML representation of a PAD1 or PADN.
 * @param t a pointer to the TLV
 * @param type the TLV's type
 * @param len the TLV's length
 * @param off the TLV's offset in the dazibao
 * @param html a preallocated string to store the TLV representation
 * @return 0 on success
 **/
int empty_pad_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html);

/**
 * Make a NULL-terminated HTML representation of a TLV.
 * @param dz the dazibao
 * @param t a pointer to the TLV
 * @param off the TLV's offset
 * @param html a pointer to a string which will be allocated to store the TLV's
 * representation
 * @return 0 on success
 **/
int tlv2html(dz_t dz, tlv_t *t, off_t off, char **html);

/**
 * Make a NULL-terminated HTML representation of a dazibao.
 * @param dz the dazibao
 * @param html a pointer to a string which will be allocated to store the
 * dazibao's representation
 * @return 0 on success
 **/
int dz2html(dz_t dz, char **html);

#endif
