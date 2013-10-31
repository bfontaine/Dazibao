#include "dazibao.h"
#include "tlv.h"

size_t tlv_read_text(wchar_t *dest, const struct tlv *t, size_t len) {
        return swprintf(dest, len, L"%s", t->value);
}
