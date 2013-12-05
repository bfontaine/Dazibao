#include <string.h>
#include "html.h"
#include "dazibao.h"
#include "tlv.h"
#include "webutils.h"

#define HTML_TLV_SIZE 1024

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

int empty_pad_tlv2html(tlv_t *t, int type, unsigned int len, char *html) {
        char fmt[] = HTML_TLV_FMT("<span>(empty)</span>");

        return snprintf(html, HTML_TLV_SIZE, fmt,
                        tlv_type2str(type), type, len);
}

int tlv2html(dz_t dz, tlv_t *t, off_t off, char **html) {
        int st, type, len;
        char text_fmt[] = HTML_TLV_FMT("");

        if (dz_read_tlv(&dz, t, off) < 0) {
                WLOGERROR("Cannot read TLV at offset %li", off);
                return -1;
        }

        *html = (char*)malloc(sizeof(char)*HTML_TLV_SIZE);

        if (*html == NULL) {
                WLOGERROR("Cannot malloc enough to generate HTML " \
                                "for the TLV at %li", off);
                perror("malloc");
                return -1;
        }

        type = tlv_get_type(*t);
        len  = type == TLV_PAD1 ? 0 : tlv_get_length(*t);

        /* TODO generate proper HTML for each TLV type */
        switch (type) {
                case TLV_PAD1:
                case TLV_PADN:
                        if (WSERVER.debug) {
                                st = empty_pad_tlv2html(t, type, len, *html);
                                break;
                        }
                case TLV_TEXT:
                        st = text_tlv2html(t, type, len, *html);
                        break;
                case TLV_PNG:
                        st = img_tlv2html(t, type, len, off, *html, PNG_EXT);
                        break;
                case TLV_JPEG:
                        st = img_tlv2html(t, type, len, off, *html, JPEG_EXT);
                        break;
                default:
                        st = snprintf(*html, HTML_TLV_SIZE, text_fmt,
                                        type, len);
        }

        if (st > HTML_TLV_SIZE) {
                WLOGWARN("sprintf of TLV at %lu failed (truncated)", off);
        }

        return 0;
}

int dz2html(dz_t dz, char **html) {
        char **tlv_html = (char**)malloc(sizeof(char*));
        char *tmp_ptr;
        off_t tlv_off;
        int html_len,
            tlv_html_len;
        tlv_t *t = (tlv_t*)malloc(sizeof(tlv_t));

        if (tlv_html == NULL || t == NULL || tlv_init(t) < 0) {
                WLOGERROR("Cannot initialize TLV");
                free(t);
                free(tlv_html);
                return -1;
        }

        *tlv_html = NULL;

        /* We may be able to optimize memory allocation here */
        html_len = strlen(HTML_DZ_TOP_FMT) + HTML_DZ_MAX_NAME_LENGTH \
                        + strlen(HTML_DZ_BOTTOM);

        *html = (char*)malloc(sizeof(char)*html_len);
        if (*html == NULL) {
                WLOGERROR("Cannot allocate enough memory for the dazibao");
                perror("malloc");
                free(t);
                free(tlv_html);
                return -1;
        }
        if (snprintf(*html, html_len, HTML_DZ_TOP_FMT, \
                                WSERVER.dzname) > html_len) {
                WLOGDEBUG("Dazibao name truncated.");
        }

        while ((tlv_off = dz_next_tlv(&dz, t)) > 0) {
                int tlv_type = tlv_get_type(*t);
                if (!WSERVER.debug && TLV_IS_EMPTY_PAD(tlv_type)) {
                        WLOGDEBUG("TLV at %li is a pad1/padN, skipping.",
                                        tlv_off);
                        continue;
                }

                if (tlv2html(dz, t, tlv_off, tlv_html) < 0) {
                        WLOGWARN("Error while reading TLV at %li, skipping.",
                                        tlv_off);
                        /*NFREE(tlv_html);*/
                        continue;
                }

                tlv_html_len = strlen(*tlv_html);
                html_len += tlv_html_len;

                WLOGDEBUG("Called tlv2html on offset %lu, got %d chars",
                                tlv_off, tlv_html_len);

                /* TODO: optimize me (realloc everytime) */
                tmp_ptr = (char*)realloc(*html, html_len);

                if (tmp_ptr == NULL) {
                        WLOGWARN("Cannot realloc, skipping tlv %lu", tlv_off);
                        perror("realloc");
                        *html = tmp_ptr;
                        NFREE(*tlv_html);
                        continue;
                }
                *html = tmp_ptr;

                strncat(*html, *tlv_html, tlv_html_len);

                /*tlv_destroy(t);
                t = NULL;*/
                NFREE(*tlv_html);
        }
        if (tlv_off == -1) {
                WLOGERROR("got an error when reading dazibao with next_tlv");
        }

        tlv_destroy(t);
        NFREE(tlv_html);

        strncat(*html, HTML_DZ_BOTTOM, strlen(HTML_DZ_BOTTOM));

        return tlv_off == -1 ? -1 : 0;
}
