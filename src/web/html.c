#include <string.h>
#include "html.h"
#include "dazibao.h"
#include "tlv.h"
#include "webutils.h"

/** @file */

int html_ensure_length(char **html, int *htmlsize, int *htmlcursor, int len) {
        if (*htmlcursor + len < *htmlsize) {
                return 0;
        }

        *htmlsize = *htmlcursor + len + HTML_CHUNK_SIZE;
        *html = (char*)safe_realloc(*html, sizeof(char)*(*htmlsize));
        if (*html == NULL) {
                perror("realloc");
                LOGERROR("cannot allocate enough memory to fit the HTML");
                return -1;
        }

        return 0;
}

int html_add_text_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor) {
        static const char fmt[] = "<blockquote>%.*s</blockquote>";
        static int fmtlen = 0;
        int w, tlen, len;
        if (fmtlen == 0) {
                fmtlen = strlen(fmt) - 4;
        }

        tlen = tlv_get_length(*t);
        len = tlen + fmtlen + 1;

        if (html_ensure_length(html, htmlsize, htmlcursor, len) == -1) {
                return -1;
        }
        
        if (dz_read_tlv(&dz, t, *off) == -1) {
                return -1;
        }

        LOGDEBUG("Adding HTML of text TLV at offset %lu, tlen=%d, len=%d, " \
                        "htmlsize=%d, cursor=%d",
                        *off, tlen, len, *htmlsize, *htmlcursor);
        w = snprintf(*html+(*htmlcursor), len, fmt,
                        tlen, tlv_get_value_ptr(*t));
        *htmlcursor += MIN(w, len);
        return 0;
}

int html_add_img_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int *htmlsize,
                int *htmlcursor) {
        static const char fmt[] = "<img src=\"/tlv/%lu%s\" />";
        int type = tlv_get_type(*t), w;
        char *ext;

        switch (type) {
                case TLV_PNG:
                        ext = strdup(PNG_EXT);
                        break;
                case TLV_JPEG:
                        ext = strdup(JPEG_EXT);
                        break;
                default:
                        ext = strdup(DEFAULT_EXT);
                        break;
        }

        w = snprintf(*html+(*htmlcursor), HTML_CHUNK_SIZE, fmt, *off, ext);
        *htmlcursor += MIN(w, HTML_CHUNK_SIZE);
         
        free(ext);
        return 0;
}

int html_add_pad1padn_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor) {
        int type = tlv_get_type(*t),
            w;
        
        w = snprintf(*html+(*htmlcursor), HTML_CHUNK_SIZE, "(%s)",
                        type == TLV_PAD1 ? "pad1" : "padN");
        *htmlcursor += MIN(w, HTML_CHUNK_SIZE);
        return 0;
}

int html_add_compound_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor) {
        return 0;
}

int html_add_dated_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor) {
        return 0;
}

int html_add_tlv(dz_t dz, tlv_t *t, off_t *dz_off, char **html, int *htmlsize,
                int *htmlcursor) {
        int tlv_type = tlv_get_type(*t),
            st = 0, written;

        if (!TLV_VALID_TYPE(tlv_type)) {
                LOGDEBUG("Unknown TLV type: %d", tlv_type);
                return 0;
        }

        if (TLV_IS_EMPTY_PAD(tlv_type) && !WSERVER.debug) {
                LOGDEBUG("Skipping Pad1/PadN at offset %lu", *dz_off);
                return 0;
        }

        if (html_ensure_length(html, htmlsize, htmlcursor,
                                HTML_TLV_MIN_SIZE) == -1) {
                tlv_destroy(t);
                LOGERROR("Cannot preallocate enough memory to " \
                                "fit a TLV");
                return -1;
        }

        written = snprintf(*html+(*htmlcursor), *htmlsize - (*htmlcursor),
                        HTML_TLV_TOP_FMT,
                        *dz_off, tlv_get_length(*t), tlv_type,
                        tlv_type2str(tlv_type));
        if (written > *htmlsize) {
                perror("snprintf");
                tlv_destroy(t);
                NFREE(*html);
                LOGERROR("snprintf of TLV top failed");
                return -1;
        }
        *htmlcursor += written;

        switch (tlv_type) {
                case TLV_PAD1:
                case TLV_PADN:
                        st = html_add_pad1padn_tlv(dz, t, dz_off,
                                        html, htmlsize, htmlcursor);
                        break;
                case TLV_TEXT:
                        st = html_add_text_tlv(dz, t, dz_off,
                                        html, htmlsize, htmlcursor);
                        break;
                case TLV_PNG:
                case TLV_JPEG:
                        st = html_add_img_tlv(dz, t, dz_off,
                                        html, htmlsize, htmlcursor);
                        break;
                case TLV_DATED:
                        st = html_add_dated_tlv(dz, t, dz_off,
                                        html, htmlsize, htmlcursor);
                case TLV_COMPOUND:
                        st = html_add_compound_tlv(dz, t, dz_off,
                                        html, htmlsize, htmlcursor);
                        break;
        };

        if (st != 0) {
                free(*html);
                tlv_destroy(t);
                LOGERROR("Something went wrong with a html_add_*");
                return -1;
        }

        if (html_ensure_length(html, htmlsize, htmlcursor,
                                strlen(HTML_TLV_BOTTOM)) == -1) {
                tlv_destroy(t);
                LOGERROR("Cannot preallocate enough memory to fit " \
                                "the HTML bottom of a TLV");
                return -1;
        }

        /* TODO memcpy here? */
        written = snprintf(*html+(*htmlcursor), *htmlsize-(*htmlcursor),
                        HTML_TLV_BOTTOM);
        if (written > *htmlsize) {
                perror("snprintf");
                tlv_destroy(t);
                NFREE(*html);
                LOGERROR("snprintf of TLV bottom failed");
                return -1;
        }
        *htmlcursor += written - 1;
        return 0;
}

int dz2html(dz_t dz, char **html) {
        tlv_t *t = (tlv_t*)malloc(sizeof(tlv_t));
        off_t dz_off;
        int htmlsize = 0,
            htmlcursor = 0,
            written = 0,
            html_bottom_len;
        
        if (dz < 0 || html == NULL || tlv_init(t) < 0) {
                tlv_destroy(t);
                LOGERROR("dz<0 or html==NULL or tlv_init failed");
                return -1;
        }

        /* We allocate more memory to avoid repetitive 'realloc' calls */
        htmlsize = strlen(HTML_DZ_TOP_FMT) + HTML_DZ_MAX_NAME_LENGTH \
                        + strlen(HTML_DZ_BOTTOM) + 2 * HTML_CHUNK_SIZE;
        htmlcursor = 0;

        *html = (char*)malloc(sizeof(char)*htmlsize);
        if (*html == NULL) {
                perror("malloc");
                tlv_destroy(t);
                LOGERROR("cannot preallocate enough memory to fit the HTML");
                return -1;
        }

        written = snprintf(*html+htmlcursor, htmlsize-htmlcursor,
                        HTML_DZ_TOP_FMT, WSERVER.dzname);
        if (written > htmlsize) {
                perror("snprintf");
                tlv_destroy(t);
                NFREE(*html);
                LOGERROR("HTML top snprintf failed, written=%d", written);
                return -1;
        }
        htmlcursor += written;

        while ((dz_off = dz_next_tlv(&dz, t)) > 0) {
                if (html_add_tlv(dz, t, &dz_off, html, &htmlsize,
                                        &htmlcursor) != 0) {
                        return -1;
                }
        }

        tlv_destroy(t);

        html_bottom_len = strlen(HTML_DZ_BOTTOM);

        if (html_ensure_length(html, &htmlsize, &htmlcursor,
                                html_bottom_len + 1) == -1) {
                tlv_destroy(t);
                LOGERROR("cannot allocate enough memory to fit the " \
                                "HTML bottom");
                return -1;
        }

        strncpy(*html+htmlcursor, HTML_DZ_BOTTOM, html_bottom_len + 1);
        LOGDEBUG("Generated %d chars of HTML", htmlcursor);

        return 0;
}
