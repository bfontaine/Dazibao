#ifndef _HTML_H
#define _HTML_H 1

/** @file
 * Utilities to generate HTML code
 **/

#include "mdazibao.h"
#include "tlv.h"
#include "string.h"

/**
 * size of memory chunks that are allocated to store the HTML representation
 * of a Dazibao
 **/
#define HTML_CHUNK_SIZE 1024

/** maximum length of a Dazibao filename in an HTML page */
#define HTML_DZ_MAX_NAME_LENGTH 64

/** maximum size of a date string */
#define HTML_MAX_DATE_SIZE 64

#define HTML_RFC_DATE "%FT%T"
#define HTML_TXT_DATE "%F, %T"

/** Format used for the HTML top of a dazibao */
#define HTML_DZ_TOP_FMT \
                  "<!doctype html>" \
                  "<html lang=\"en\" dir=\"ltr\">" \
                    "<head>" \
                      "<meta charset=\"utf-8\" />" \
                      "<meta name=\"language\" content=\"fr\" />" \
                      "<link rel=\"stylesheet\" href=\"/dz.css\">" \
                      "<title>Dazibao - %s</title>" \
                    "</head>" \
                    "<body>" \
                      "<div class=\"dazibao\">" \
                        "<ol class=\"tlvs\">"
/** Format used for the HTML top of a TLV */
#define HTML_TLV_TOP_FMT \
                          "<li class=\"tlv\" data-offset=\"%li\"" \
                                           " data-length=\"%u\"" \
                                           " data-type=\"%d\">" \
                            "<span class=\"type\">%s</span>" \
                            "<div class=\"value\">"
/** Format used for the HTML bottom of a TLV */
#define HTML_TLV_BOTTOM     "</div>" \
                          "</li>"
/** Format used for the HTML bottom of a dazibao */
#define HTML_DZ_BOTTOM \
                        "</ol>" \
                      "</div>" \
                      "<script src=\"/dz.js\"></script>" \
                    "</body>" \
                  "</html>"

/** HTML format for a text TLV */
#define HTML_TLV_TEXT_FMT "<blockquote>%.*s</blockquote>"
/** HTML format for an image TLV */
#define HTML_TLV_IMG_FMT "<img src=\"/tlv/%li\" />"
/** HTML format for an image TLV with height/width attributes */
#define HTML_TLV_IMG_DIMS_FMT \
        "<img src=\"/tlv/%li\" height=\"%d\" width=\"%d\" />"
/** HTML format for the top of a dated TLV */
#define HTML_TLV_DATED_TOP_FMT "<time datetime=\"%s\">%s</time>" \
                                 "<ol class=\"subtlv\">"
/** HTML format for the bottom of a dated TLV */
#define HTML_TLV_DATED_BOTTOM_FMT "</ol>"
/** HTML format for the top of a compound TLV */
#define HTML_TLV_COMPOUND_TOP_FMT "<ol class=\"subtlvs\">"
/** HTML format for the bottom of a compound TLV */
#define HTML_TLV_COMPOUND_BOTTOM_FMT "</ol>"
/** HTML format for a generic TLV - offset, type string */
#define HTML_TLV_OTHER_FMT "<a href=\"/tlv/%li\" />See as %s</a>"

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
 * @param t a pointer to the TLV with its type and length
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
 * assuming its value is an image.
 * @param dz the dazibao
 * @param t a pointer to the TLV with its type and length
 * @param off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_img_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int *htmlsize,
                int *htmlcursor);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string.
 * This is not type-dependent, and you should use other functions when possible
 * to generate more user-friendly HTML.
 * @param dz the dazibao
 * @param t a pointer to the TLV with its type and length
 * @param off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_other_tlv(dz_t dz, tlv_t *t, off_t *off, char **html,
                int *htmlsize, int *htmlcursor);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string,
 * assuming it's a Pad1 or PadN
 * @param t a pointer to the TLV with its type and length
 * @param html a pointer to an HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_pad1padn_tlv(tlv_t *t, char **html, int *htmlcursor);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string,
 * assuming it's a compound one
 * @param dz the dazibao
 * @param t a pointer to the TLV with its type and length
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
 * @param t a pointer to the TLV with its type and length
 * @param off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_dated_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor);

/**
 * Add a non-NULL-terminated HTML representation of a TLV to an HTML string.
 * @param dz the dazibao
 * @param t a pointer to the pre-filled TLV (type + length)
 * @param dz_off a pointer to the TLV's offset in the dazibao
 * @param html a pointer to an HTML string
 * @param htmlsize a pointer to the size of the HTML string
 * @param htmlcursor the current cursor in the HTML string
 * @return 0 on success
 **/
int html_add_tlv(dz_t dz, tlv_t *t, off_t *dz_off, char **html, int *htmlsize,
                int *htmlcursor);

/**
 * Make a NULL-terminated HTML representation of a dazibao.
 * @param dz the dazibao
 * @param html a pointer to a string which will be allocated to store the
 * dazibao's representation
 * @return 0 on success
 **/
int dz2html(dz_t dz, char **html);

#endif
