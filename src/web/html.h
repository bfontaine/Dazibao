#ifndef _HTML_H
#define _HTML_H 1

/** @file
 * Utilities to generate HTML code
 **/

#include "dazibao.h"
#include "tlv.h"
#include "string.h"

/**
 * size of memory chunks that are allocated to store the HTML representation
 * of a Dazibao
 **/
#define HTML_CHUNK_SIZE 1024

/** maximum length of a Dazibao filename in an HTML page */
#define HTML_DZ_MAX_NAME_LENGTH 64

/** Format used for the HTML top of a dazibao */
#define HTML_DZ_TOP_FMT \
                  "<!doctype html>\n" \
                  "<html lang=\"en\" dir=\"ltr\">\n" \
                  "  <head>\n" \
                  "    <meta charset=\"utf-8\" />\n" \
                  "    <meta name=\"language\" content=\"fr\" />\n" \
                  "    <link rel=\"stylesheet\" href=\"/dz.css\">\n" \
                  "    <title>Dazibao - %s</title>\n" \
                  "  </head>\n" \
                  "  <body>\n" \
                  "    <ol class=\"tlvs\">\n"
/** Format used for the HTML top of a TLV */
#define HTML_TLV_TOP_FMT \
                  "      <li class=\"tlv\" data-offset=\"%lu\"" \
                                         " data-length=\"%u\"" \
                                         " data-type=\"%d\">\n" \
                  "        <span class=\"type\">%s</span>\n" \
                  "        <div class=\"value\">\n"
/** Format used for the HTML bottom of a TLV */
#define HTML_TLV_BOTTOM "\n" \
                  "        </div>\n" \
                  "      </li>\n\n"
/** Format used for the HTML bottom of a dazibao */
#define HTML_DZ_BOTTOM \
                  "    </ol>\n" \
                  "    <script src=\"/dz.js\"></script>\n" \
                  "  </body>\n" \
                  "</html>\n"

/** maximum size of the HTML top of a TLV */
#define HTML_TLV_TOP_MAX_LENGTH (strlen(HTML_TLV_TOP_FMT) + 64)

/**
 * the size to have to store a TLV as HTML. This may not be the exact needed
 * size, but we'll using this value to increase the HTML string enough to not
 * need to reallocate again.
 **/
#define HTML_TLV_MIN_SIZE \
        (HTML_TLV_TOP_MAX_LENGTH+strlen(HTML_TLV_BOTTOM)+HTML_CHUNK_SIZE)

/**
 * Ensure that the string pointed by 'html' of size '*htmlsize' has enough
 * memory to fit '*htmlcursor + len' bytes. On error, the string is freed
 * @param html
 * @param htmlsize
 * @param htmlcursor
 * @param len the length
 * @return 0 on success
 **/
int html_ensure_length(char **html, int *htmlsize, int *htmlcursor, int len);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string,
 * assuming its value is in plain text.
 * @param dz the dazibao
 * @param off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_text_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string,
 * assuming its value is a PNG or JP(E)G image.
 * @param dz the dazibao
 * @param off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_img_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int *htmlsize,
                int *htmlcursor);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string,
 * assuming it's a Pad1 or PadN
 * @param dz the dazibao
 * @param off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_pad1padn_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string,
 * assuming it's a compound one
 * @param dz the dazibao
 * @param off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_compound_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string,
 * assuming it's a dated one
 * @param dz the dazibao
 * @param off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_dated_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor);

/**
 * Make a NULL-terminated HTML representation of a dazibao.
 * @param dz the dazibao
 * @param html a pointer to a string which will be allocated to store the
 * dazibao's representation
 * @return 0 on success
 **/
int dz2html(dz_t dz, char **html);

#endif
