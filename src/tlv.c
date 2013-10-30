#include "dazibao.h"
#include "tlv.h"

size_t tlv_read_text(wchar_t *dest, const struct tlv *t, size_t len) {
        // TODO see if we can just memcpy a part of t->value in a
        // wchat_t*.

        mbstate_t state;
        size_t maxlen, readlen;
        char *val;
        wchar_t *dest_cursor = dest;

        memset(&state, '\0', sizeof(state));

        if (dest == NULL || t == NULL || t->value == NULL) {
                return -1;
        }

        val = t->value;
        maxlen = t->length;

        while (len > 0
                && maxlen > 0
                && (readlen = mbrtowc(dest_cursor, val, maxlen, &state))) {

                if (readlen < 0) {
                        return -1;
                }

                maxlen -= readlen;
                len--;
        }

        mbrtowc(dest_cursor, L"\0", 1, &state);

        return len;
}
