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

int tlv_get_type(tlv_t tlv) {
	return tlv[0];
}

void tlv_set_type(tlv_t tlv, char t) {
	tlv[0] = t;
}

void tlv_set_length(tlv_t tlv, unsigned int n) {
	htod(n, tlv_get_length_ptr(tlv));
}

char *tlv_get_length_ptr(tlv_t tlv) {
	return (tlv + TLV_SIZEOF_TYPE);
}

unsigned int tlv_get_length(tlv_t tlv) {
	return dtoh(tlv_get_length_ptr(tlv));
}

char *tlv_get_value_ptr(tlv_t tlv) {
	return tlv + TLV_SIZEOF_HEADER;
}


int tlv_write(tlv_t tlv, int fd) {
	/* write */
	unsigned int to_write = TLV_SIZEOF(tlv);
	if (write(fd, tlv, to_write) != to_write) {
		ERROR("write", -1);
	}
	return 0;
}

int tlv_read(tlv_t tlv, int fd) {

	int len = tlv_get_length(tlv);
	
	tlv = realloc(tlv, sizeof(*tlv) * (TLV_SIZEOF_HEADER + len));
	
	if (read(fd, tlv_get_value_ptr(tlv), len) < len) {
		ERROR("read", -1);
	}
	return 0;
}
