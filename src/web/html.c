#include <string.h>
#include "html.h"
#include "dazibao.h"
#include "tlv.h"
#include "webutils.h"

int tlv2html(dz_t dz, tlv_t t, off_t off, char **html) {
  /*      if (dz_read_tlv(&dz, &t, off) < 0) {
                WLOGERROR("Cannot read TLV at offset %li", off);
                return -1;
        }
*/
        /* TODO */
        *html = strdup("<li>one tlv</li>\n");

        /*free(t);*/
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
        *html = strdup(HTML_DZ_TOP);
        html_len = strlen(*html) + sizeof(char)*(HTML_DZ_BOTTOM_LEN+1);
        *html = safe_realloc(*html, html_len);

        while ((tlv_off = dz_next_tlv(&dz, t)) > 0) {
                if (tlv2html(dz, *t, tlv_off, tlv_html) < 0) {
                        WLOGWARN("Error while reading TLV at %li, skipping.",
                                        tlv_off);
                        /*tlv_destroy(t);*/
                        NFREE(tlv_html);
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

        strncat(*html, HTML_DZ_BOTTOM, HTML_DZ_BOTTOM_LEN);

        return tlv_off == -1 ? -1 : 0;
}
