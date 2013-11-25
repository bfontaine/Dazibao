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
        off_t tlv_off;
        int html_cursor = 0;
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
        *html = safe_realloc(*html, strlen(*html)
                                        + sizeof(char)*(HTML_DZ_BOTTOM_LEN+1));
        html_cursor += HTML_DZ_TOP_LEN; /* see stpcpy too */

        WLOGDEBUG("dazibao=%d", dz);
        while ((tlv_off = dz_next_tlv(&dz, t)) > 0) {
                if (tlv2html(dz, *t, tlv_off, tlv_html) < 0) {
                        WLOGWARN("Error while reading TLV at %li, skipping.",
                                        tlv_off);
                        /*tlv_destroy(t);*/
                        NFREE(tlv_html);
                        continue;
                }

                /* TODO: append tlv to current HTML */

                /*tlv_destroy(t);
                t = NULL;*/
                NFREE(*tlv_html);
        }
        if (tlv_off == -1) {
                WLOGERROR("got an error when reading dazibao with next_tlv");
        }

        tlv_destroy(t);
        NFREE(tlv_html);

        strncpy(*html + html_cursor, HTML_DZ_BOTTOM, HTML_DZ_BOTTOM_LEN+1);

        return tlv_off == -1 ? -1 : 0;
}
