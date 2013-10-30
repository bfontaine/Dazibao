#ifndef _TLVS_H
#define _TLVS_H 1

#include <wchar.h>

#define TLV_PAD1     0
#define TLV_PADN     1
#define TLV_TEXT     2
#define TLV_PNG      3
#define TLV_JPEG     4
#define TLV_COMPOUND 5
#define TLV_DATED    6

#define TLV_SIZEOF_TYPE 1
#define TLV_SIZEOF_LENGTH 3
#define TLV_SIZEOF_HEADER 4
#define TLV_SIZEOF(t) (TLV_SIZEOF_TYPE+((t).type==TLV_PAD1 \
                                        ? 0                \
                                        : TLV_SIZEOF_LENGTH+(t).length))

#define TLV_IS_EMPTY_PAD(t) ((t) == TLV_PAD1 || (t) == TLV_PADN)

#define TLV_MAX_VALUE_SIZE ((1<<((TLV_SIZEOF_LENGTH)*8))-1)
#define TLV_MAX_SIZE ((TLV_SIZEOF_HEADER)+(TLV_MAX_VALUE_SIZE))

/* Copy at most 'len' wide-characters into 'dest' from the value of tlv 't',
 * and return the number of wide-characters effectively copied. 'dest' is
 * NULL-terminated, and the NULL byte is not included in 'len', so allocate
 * room for at least len+1 wchar_t's in 'dest'.
 * The function returns -1 if an error occured or 'dest' and/or 't' are NULL.
 */
size_t tlv_read_text(wchar_t *dest, const struct tlv *t, size_t len);

#endif
