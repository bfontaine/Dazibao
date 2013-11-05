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
								: TLV_SIZEOF_LENGTH+dtoh((t).len)))

#define TLV_IS_EMPTY_PAD(t) ((t) == TLV_PAD1 || (t) == TLV_PADN)

#define TLV_MAX_VALUE_SIZE ((1<<((TLV_SIZEOF_LENGTH)*8))-1)
#define TLV_MAX_SIZE ((TLV_SIZEOF_HEADER)+(TLV_MAX_VALUE_SIZE))

typedef char value_t;

typedef char tlv_len[3];

struct tlv {
	unsigned char type;
	tlv_len len;
	value_t *value;
};

#endif
