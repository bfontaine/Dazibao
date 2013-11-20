#include <string.h>
#include "html.h"
#include "dazibao.h"
#include "tlv.h"
#include "webutils.h"

int tlv2html(dz_t dz, tlv_t t, off_t off, char **html) {
        if (dz_read_tlv(&dz, t, off) < 0) {
                WLOGERROR("Cannot read TLV at offset %li", off);
                return -1;
        }

        /* TODO */
        *html = strdup("one tlv");

        free(t);
        return 0;
}

int dz2html(dz_t dz, char **html) {
        char **tlv_html = (char**)malloc(sizeof(char*));
        off_t tlv_off;
        tlv_t t; /* TODO initialize */

        *html = NULL; /* TODO add beginning of HTML */

        while ((tlv_off = dz_next_tlv(&dz, t)) > 0) {
                if (tlv2html(dz, t, tlv_off, tlv_html) < 0) {
                        WLOGWARN("Error while reading TLV at %li, skipping.",
                                        tlv_off);
                        NFREE(t);
                        NFREE(tlv_html);
                        continue;
                }

                /* TODO: append tlv to current HTML */

                NFREE(t);
                NFREE(tlv_html);
        }
        if (tlv_off == -1) {
                WLOGERROR("got an error when reading dazibao with next_tlv");
        }

        NFREE(t);
        NFREE(tlv_html);
        return tlv_off == -1 ? -1 : 0;
}
