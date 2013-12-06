#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "utils.h"
#include "tlv.h"

/** @file */

/**
 * Convert {n} in dazibao's endianess
 * and set {tlv}'s length field with the converted value.
 * @param n wanted length
 * @param tlv tlv receiving length
 * @deprecated use tlv_set_length instead
 **/
static void htod(unsigned int n, char *len) {
	union {
		unsigned int i;
		unsigned char c[4];
	} tmp;
	tmp.i = htonl(n);
	memcpy(len, &tmp.c[1], 3);
}

/**
 * Convert an int written in dazibao's endianess to host endianess.
 * @param len int using dazibao's endianess
 * @return value of length
 * @deprecated use get_length
 **/
static unsigned int dtoh(char *len) {
	unsigned char *tmp = (unsigned char *)len;
	return (tmp[0] << 16) + (tmp[1] << 8) + tmp[2];
}

int tlv_init(tlv_t *t) {
        if (t == NULL) {
                return -1;
        }

        *t = (tlv_t)malloc(sizeof(char)*TLV_SIZEOF_HEADER);
        if (*t == NULL) {
                return -1;
        }
        return 0;
}

int tlv_destroy(tlv_t *t) {
        if (t == NULL) {
                return 0;
        }
        if (*t != NULL) {
                NFREE(*t);
        }
        free(t);
        return 0;
}

int tlv_get_type(tlv_t tlv) {
	return tlv[0];
}

void tlv_set_type(tlv_t *tlv, char t) {
	(*tlv)[0] = t;
}

void tlv_set_length(tlv_t *tlv, unsigned int n) {
	htod(n, tlv_get_length_ptr(*tlv));
}

char *tlv_get_length_ptr(tlv_t tlv) {
	return tlv + TLV_SIZEOF_TYPE * sizeof(char);
}

unsigned int tlv_get_length(tlv_t tlv) {
	return dtoh(tlv_get_length_ptr(tlv));
}

char *tlv_get_value_ptr(tlv_t tlv) {
	return tlv + TLV_SIZEOF_HEADER * sizeof(char);
}


int tlv_write(tlv_t tlv, int fd) {
	unsigned int to_write = TLV_SIZEOF(tlv);
        int status = write_all(fd, tlv, to_write);
	if (status == -1 || (unsigned int)status != to_write) {
		ERROR("write", -1);
	}
	return 0;
}

int tlv_read(tlv_t *tlv, int fd) {

	int len = tlv_get_length(*tlv);
	
	*tlv = (tlv_t)safe_realloc(*tlv, sizeof(char)
                                                * (TLV_SIZEOF_HEADER + len));

	if (*tlv == NULL) {
		ERROR("realloc", -1);
	}

	if (read(fd, tlv_get_value_ptr(*tlv), len) < len) {
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
