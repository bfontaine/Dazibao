#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "utils.h"
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
	unsigned int to_write = TLV_SIZEOF(tlv);
        int status = write(fd, tlv, to_write);
	if (status == -1 || (unsigned int)status != to_write) {
		ERROR("write", -1);
	}
	return 0;
}

int tlv_read(tlv_t tlv, int fd) {

	int len = tlv_get_length(tlv);
	
	tlv = safe_realloc(tlv, sizeof(*tlv) * (TLV_SIZEOF_HEADER + len));

	if (tlv == NULL) {
		ERROR("realloc", -1);
	}

	if (read(fd, tlv_get_value_ptr(tlv), len) < len) {
		ERROR("read", -1);
	}
	return 0;
}


int dump_tlv(tlv_t tlv, int fd) {
	return write(fd, tlv, TLV_SIZEOF(tlv));
}

int dump_tlv_value(tlv_t tlv, int fd) {
	return write(fd, tlv_get_value_ptr(tlv), tlv_get_length(tlv));
}

const char *tlv_type2str(char tlv_type) {
        switch (tlv_type) {
        case TLV_PAD1:  return "pad1";
        case TLV_PADN:  return "padN";
        case TLV_TEXT:  return "text";
        case TLV_PNG:   return "png";
        case TLV_JPEG:  return "jpeg";
        case TLV_COMPOUND:
                        return "compound";
        case TLV_DATED: return "dated";
        default:
                        return "unknown";
        }
}
