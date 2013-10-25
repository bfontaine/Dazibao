#ifndef _TLVS_H
#define _TLVS_H 1

#define TLV_PAD1     0
#define TLV_PADN     1
#define TLV_TEXT     2
#define TLV_PNG      3
#define TLV_JPEG     4
#define TLV_COMPOUND 5
#define TLV_DATED    6

#define SIZEOF_TLV(t) (4+t.length)

#endif
