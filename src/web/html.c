#include <string.h>
#include "html.h"
#include "mdazibao.h"
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
        int w, tlen, len;

        tlen = tlv_get_length(t);
        len = tlen + (strlen(HTML_TLV_TEXT_FMT) - 4) + 1;

        if (html_ensure_length(html, htmlsize, htmlcursor, len) == -1) {
                return -1;
        }

        SAVE_OFFSET(dz);
        if (dz_read_tlv(&dz, t, *off) < 0) {
                return -1;
        }
        RESTORE_OFFSET(dz);

        LOGDEBUG("Adding HTML of text TLV at offset %lu, tlen=%d, len=%d, " \
                        "htmlsize=%d, cursor=%d",
                        *off, tlen, len, *htmlsize, *htmlcursor);
        w = snprintf(*html+(*htmlcursor), len, HTML_TLV_TEXT_FMT,
                        tlen, tlv_get_value_ptr(t));
        *htmlcursor += MIN(w, len);
        return 0;
}

int html_add_img_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int *htmlsize,
                int *htmlcursor) {
        int type = tlv_get_type(t), w;
        char *ext;

        switch (type) {
                case TLV_PNG:
                        ext = PNG_EXT;
                        break;
                case TLV_JPEG:
                        ext = JPEG_EXT;
                        break;
                default:
                        ext = DEFAULT_EXT;
                        break;
        }

        LOGDEBUG("Adding HTML of img TLV (ext=%s) at offset %lu.", ext, *off);

        w = snprintf(*html+(*htmlcursor), HTML_CHUNK_SIZE,
                        HTML_TLV_IMG_FMT, *off, ext);
        *htmlcursor += MIN(w, HTML_CHUNK_SIZE);

        return 0;
}

int html_add_pad1padn_tlv(tlv_t *t, char **html, int *htmlcursor) {
        int type = tlv_get_type(t),
            w;

        LOGDEBUG("Adding HTML of pad1/padN (type=%d)", type);

        w = snprintf(*html+(*htmlcursor), HTML_CHUNK_SIZE, "(%s)",
                        type == TLV_PAD1 ? "pad1" : "padN");
        *htmlcursor += MIN(w, HTML_CHUNK_SIZE);
        return 0;
}

int html_add_compound_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor) {

        int tlen = tlv_get_length(t),
            len_top = strlen(HTML_TLV_COMPOUND_TOP_FMT),
            len_bottom = strlen(HTML_TLV_COMPOUND_BOTTOM_FMT);
        off_t off_value = *off + TLV_SIZEOF_HEADER,
              off_after = off_value + tlen,
              dz_off;

        memcpy(*html+(*htmlcursor), HTML_TLV_COMPOUND_TOP_FMT, len_top);
        *htmlcursor += len_top;

        LOGDEBUG("Adding a TLV compound of length=%d at offset %lu", tlen,
                        *off);

        if (tlen > 0) {
                SET_OFFSET(dz, off_value);
                *off = off_value;
                while ((dz_off = dz_next_tlv(&dz, t)) > 0) {
                        if (html_add_tlv(dz, t, &dz_off, html, htmlsize,
                                                htmlcursor) != 0) {
                                return -1;
                        }
                }
        }

        memcpy(*html+(*htmlcursor), HTML_TLV_COMPOUND_BOTTOM_FMT, len_bottom);
        *htmlcursor += len_bottom;
        *off = off_after;
        /*SET_OFFSET(dz, off_after);*/
        return 0;
}

int html_add_dated_tlv(dz_t dz, tlv_t *t, off_t *off, char **html, int
                *htmlsize, int *htmlcursor) {
        time_t time;
        struct tm date;
        char *time_iso = NULL,
             *time_txt = NULL;
        int len = tlv_get_length(t),
            len_bottom, w;
        off_t off_value = *off + TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE,
              off_after = *off + TLV_SIZEOF_HEADER + len;

        time = dz_read_date_at(&dz, *off + TLV_SIZEOF_HEADER);

        /* FIXME check for wrong dates */

        time_iso = (char*)malloc(sizeof(char)*HTML_MAX_DATE_SIZE);
        time_txt = (char*)malloc(sizeof(char)*HTML_MAX_DATE_SIZE);

        if (time_iso == NULL || time_txt == NULL) {
                NFREE(time_iso);
                NFREE(time_txt);
                return -1;
        }

        memset(&date, 0, sizeof(date));
        gmtime_r(&time, &date);

        strftime(time_iso, HTML_MAX_DATE_SIZE, HTML_RFC_DATE, &date);
        strftime(time_txt, HTML_MAX_DATE_SIZE, HTML_TXT_DATE, &date);

        w = snprintf(*html+(*htmlcursor), HTML_CHUNK_SIZE,
                        HTML_TLV_DATED_TOP_FMT, time_iso, time_txt);
        *htmlcursor += MIN(w, HTML_CHUNK_SIZE);

        *off = off_value;

        if (len > 0) {
                int st = dz_tlv_at(&dz, t, *off);
                st |= html_add_tlv(dz, t, off, html, htmlsize, htmlcursor);
                if (st == -1) {
                        return -1;
                }
        }

        len_bottom = strlen(HTML_TLV_DATED_BOTTOM_FMT);
        memcpy(*html+(*htmlcursor), HTML_TLV_DATED_BOTTOM_FMT, len_bottom);
        (*htmlcursor) += len_bottom;

        free(time_iso);
        free(time_txt);
        *off = off_after;
        return 0;
}

int html_add_tlv(dz_t dz, tlv_t *t, off_t *dz_off, char **html, int *htmlsize,
                int *htmlcursor) {
        int tlv_type = tlv_get_type(t),
            st = 0, written,
            len_bottom;

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
                        *dz_off, tlv_get_length(t), tlv_type,
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
                        st = html_add_pad1padn_tlv(t, html, htmlcursor);
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
                        break;
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

        len_bottom = strlen(HTML_TLV_BOTTOM);
        memcpy(*html+(*htmlcursor), HTML_TLV_BOTTOM, len_bottom);
        *htmlcursor += len_bottom;
        return 0;
}

int dz2html(dz_t dz, char **html) {
        tlv_t *t = (tlv_t*)malloc(sizeof(tlv_t));
        off_t dz_off;
        int htmlsize = 0,
            htmlcursor = 0,
            written = 0,
            html_bottom_len;

        if (dz.fd < 0 || html == NULL || tlv_init(t) < 0) {
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
