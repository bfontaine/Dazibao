#ifndef _TLVS_H
#define _TLVS_H 1

#include <arpa/inet.h>
#include <string.h>

#define TLV_PAD1     0
#define TLV_PADN     1
#define TLV_TEXT     2
#define TLV_PNG      3
#define TLV_JPEG     4
#define TLV_COMPOUND 5
#define TLV_DATED    6

#define TLV_SIZEOF_TYPE 1
#define TLV_SIZEOF_LENGTH 3
#define TLV_SIZEOF_HEADER (TLV_SIZEOF_TYPE + TLV_SIZEOF_LENGTH)
#define TLV_SIZEOF(t) (TLV_SIZEOF_TYPE+(tlv_get_type(t)==TLV_PAD1 \
                                ? 0                               \
                                : TLV_SIZEOF_LENGTH+tlv_get_length((t))))

#define TLV_IS_EMPTY_PAD(t) ((t) == TLV_PAD1 || (t) == TLV_PADN)

#define TLV_MAX_VALUE_SIZE ((1<<((TLV_SIZEOF_LENGTH)*8))-1)
#define TLV_MAX_SIZE ((TLV_SIZEOF_HEADER)+(TLV_MAX_VALUE_SIZE))

// TODO doc

void htod(unsigned int n, char *tlv);

unsigned int dtoh(char *len);

int tlv_get_type(char *tlv);

void tlv_set_type(char *tlv, char t);

void tlv_set_length(char *tlv, unsigned int n);

char *tlv_get_length_ptr(char *tlv);

unsigned int tlv_get_length(char *tlv);

char *tlv_get_value_ptr(char *tlv);

#endif
