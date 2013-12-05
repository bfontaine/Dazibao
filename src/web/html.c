#include <string.h>
#include "html.h"
#include "dazibao.h"
#include "tlv.h"
#include "webutils.h"

/** @file */

int text_tlv2html(tlv_t *t, int type, unsigned int len, char *html) {
        char fmt[] = HTML_TLV_FMT("<blockquote>%.*s</blockquote>");

        return snprintf(html, HTML_TLV_SIZE, fmt, tlv_type2str(type),
                        type, len, len, tlv_get_value_ptr(*t));
}

int img_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html, const char *ext) {
        char fmt[] = HTML_TLV_FMT("<img src=\"/tlv/%li%s\" />");

        return snprintf(html, HTML_TLV_SIZE, fmt,
                        tlv_type2str(type), type, len, off, ext);
}

int dated_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html) {
        /* TODO HTML to dated TLVs */
        snprintf(html, HTML_TLV_SIZE, HTML_TLV_FMT("(dated)"),
                        tlv_type2str(type), type, len);
        return -1;
}
int compound_tlv2html(tlv_t *t, int type, unsigned int len, off_t off,
                char *html) {
        /* TODO HTML to compound TLVs */
        snprintf(html, HTML_TLV_SIZE, HTML_TLV_FMT("(compound)"),
                        tlv_type2str(type), type, len);
        return -1;
}

int empty_pad_tlv2html(tlv_t *t, int type, unsigned int len, char *html) {
        char fmt[] = HTML_TLV_FMT("<span>(empty)</span>");

        return snprintf(html, HTML_TLV_SIZE, fmt,
                        tlv_type2str(type), type, len);
}

int tlv2html(dz_t dz, tlv_t *t, off_t off, char **html) {
        int st, type, len;
        char text_fmt[] = HTML_TLV_FMT("");

        if (dz_read_tlv(&dz, t, off) < 0) {
                LOGERROR("Cannot read TLV at offset %li", off);
                return -1;
        }

        *html = (char*)malloc(sizeof(char)*HTML_TLV_SIZE);

        if (*html == NULL) {
                LOGERROR("Cannot malloc enough to generate HTML " \
                                "for the TLV at %li", off);
                perror("malloc");
                return -1;
        }

        type = tlv_get_type(*t);
        len  = type == TLV_PAD1 ? 0 : tlv_get_length(*t);

        switch (type) {
                case TLV_PAD1:
                case TLV_PADN:
                        st = empty_pad_tlv2html(t, type, len, *html);
                        break;
                case TLV_TEXT:
                        st = text_tlv2html(t, type, len, *html);
                        break;
                case TLV_PNG:
                        st = img_tlv2html(t, type, len, off, *html, PNG_EXT);
                        break;
                case TLV_JPEG:
                        st = img_tlv2html(t, type, len, off, *html, JPEG_EXT);
                        break;
                case TLV_DATED:
                        st = dated_tlv2html(t, type, len, off, *html);
                        break;
                case TLV_COMPOUND:
                        st = compound_tlv2html(t, type, len, off, *html);
                        break;
                default:
                        st = snprintf(*html, HTML_TLV_SIZE, text_fmt,
                                        tlv_type2str(type), type, len);
        }

        if (st > HTML_TLV_SIZE) {
                LOGWARN("sprintf of TLV at %lu failed (truncated)", off);
        }

        return 0;
}

int dz2html(dz_t dz, char **html) {
        char **tlv_html = (char**)malloc(sizeof(char*));
        off_t tlv_off;
        int html_len,
            tlv_html_len,
            preallocated_len;

        tlv_t *t = (tlv_t*)malloc(sizeof(tlv_t));

        if (tlv_html == NULL || t == NULL || tlv_init(t) < 0) {
                LOGERROR("Cannot initialize TLV");
                free(t);
                free(tlv_html);
                return -1;
        }
        *tlv_html = NULL;

        html_len = strlen(HTML_DZ_TOP_FMT) + HTML_DZ_MAX_NAME_LENGTH \
                        + strlen(HTML_DZ_BOTTOM);

        /* preallocating more memory to avoid repetitive 'realloc' calls */
        preallocated_len = html_len + 2 * HTML_TLV_SIZE;

        *html = (char*)malloc(sizeof(char)*preallocated_len);
        if (*html == NULL) {
                LOGERROR("Cannot allocate enough memory for the dazibao");
                perror("malloc");
                free(t);
                free(tlv_html);
                return -1;
        }
        if (snprintf(*html, html_len, HTML_DZ_TOP_FMT, \
                                WSERVER.dzname) > html_len) {
                LOGDEBUG("Dazibao name truncated.");
        }

        while ((tlv_off = dz_next_tlv(&dz, t)) > 0) {
                int tlv_type = tlv_get_type(*t);
                if (!WSERVER.debug && TLV_IS_EMPTY_PAD(tlv_type)) {
                        LOGDEBUG("TLV at %li is a pad1/padN, skipping.",
                                        tlv_off);
                        continue;
                }

                NFREE(*tlv_html);
                if (tlv2html(dz, t, tlv_off, tlv_html) < 0) {
                        LOGWARN("Error while reading TLV at %li, skipping.",
                                        tlv_off);
                        continue;
                }

                tlv_html_len = strlen(*tlv_html);
                html_len += tlv_html_len;

                LOGDEBUG("Called tlv2html on offset %lu, got %d chars",
                                tlv_off, tlv_html_len);

                if (html_len > preallocated_len) {
                        char *tmp_ptr = (char*)realloc(*html, html_len);

                        if (tmp_ptr == NULL) {
                                LOGWARN("Cannot realloc, skipping tlv %lu",
                                                tlv_off);
                                perror("realloc");
                                continue;
                        }
                        *html = tmp_ptr;
                } else {
                        LOGDEBUG("Enough preallocated memory for this TLV");
                }

                strncat(*html, *tlv_html, tlv_html_len);

                /*tlv_destroy(t);
                t = NULL;*/
        }
        if (tlv_off == -1) {
                LOGERROR("got an error when reading dazibao with next_tlv");
        }

        tlv_destroy(t);
        NFREE(*tlv_html);
        NFREE(tlv_html);

        strncat(*html, HTML_DZ_BOTTOM, strlen(HTML_DZ_BOTTOM));

        return tlv_off == -1 ? -1 : 0;
}
