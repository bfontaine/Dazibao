#ifndef _TLVS_H
#define _TLVS_H 1

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "utils.h"


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

typedef char* tlv_t;

// TODO doc

void htod(unsigned int n, tlv_t tlv);

unsigned int dtoh(char *len);

int tlv_get_type(tlv_t tlv);

void tlv_set_type(tlv_t tlv, char t);

void tlv_set_length(tlv_t tlv, unsigned int n);

tlv_t tlv_get_length_ptr(tlv_t tlv);

unsigned int tlv_get_length(tlv_t tlv);

tlv_t tlv_get_value_ptr(tlv_t tlv);

int tlv_write(tlv_t tlv, int fd);

#endif
