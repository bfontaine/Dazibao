#include "tlv.h"

void htod(unsigned int n, char *len) {
	union {
		unsigned int i;
		unsigned char c[4];
	} tmp;
	tmp.i = htonl(n);
	memcpy(len, &tmp.c[1], 3);
}

unsigned int dtoh(char *len) {
	unsigned char *tmp = (unsigned char *)len;
	return (tmp[0] << 16) + (tmp[1] << 8) + tmp[2];
}

int tlv_get_type(char *tlv) {
	return tlv[0];
}

void tlv_set_type(char *tlv, char t) {
	tlv[0] = t;
}

void tlv_set_length(char *tlv, unsigned int n) {
	htod(n, tlv_get_length_ptr(tlv));
}

char *tlv_get_length_ptr(char *tlv) {
	return (tlv + TLV_SIZEOF_TYPE);
}

unsigned int tlv_get_length(char *tlv) {
	return dtoh(tlv_get_length_ptr(tlv));
}

char *tlv_get_value_ptr(char *tlv) {
	return tlv + TLV_SIZEOF_HEADER;
}
