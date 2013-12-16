#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils.h"
#include "tlv.h"
#include "logging.h"

/** @file */

struct tlv_type tlv_types[] = {
        { TLV_PAD1     , "pad1"     },
        { TLV_PADN     , "padN"     },
        { TLV_TEXT     , "text"     },
        { TLV_PNG      , "png"      },
        { TLV_JPEG     , "jpg"      },
        { TLV_COMPOUND , "compound" },
        { TLV_DATED    , "dated"    },
        { TLV_GIF      , "gif"      },
        {-1,NULL}
};

/**
 * Convert {n} in dazibao's endianess
 * and set {tlv}'s length field with the converted value.
 * @param n wanted length
 * @param tlv tlv receiving length
 * @deprecated use tlv_set_length instead
 **/
void htod(unsigned int n, char *len) {
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

void tlv_set_date(tlv_t *tlv, uint32_t date) {
        uint32_t d = htonl(date);
        memcpy((*tlv) + TLV_SIZEOF_HEADER, &d, sizeof(d));
}

int tlv_init(tlv_t *t) {
        if (t == NULL) {
                return -1;
        }

        *t = (tlv_t)malloc(sizeof(char) * TLV_SIZEOF_HEADER);
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
        return 0;
}

int tlv_get_type(tlv_t *tlv) {
        return (unsigned char)((*tlv)[0]);
}

void tlv_set_type(tlv_t *tlv, unsigned char t) {
        (*tlv)[0] = t;
}

void tlv_set_length(tlv_t *tlv, unsigned int n) {
        htod(n, tlv_get_length_ptr(tlv));
}

char *tlv_get_length_ptr(tlv_t *tlv) {
        return *tlv + TLV_SIZEOF_TYPE;
}

unsigned int tlv_get_length(tlv_t *tlv) {
        return tlv_get_type(tlv) == TLV_PAD1
                ? 0 : dtoh(tlv_get_length_ptr(tlv));
}

char *tlv_get_value_ptr(tlv_t *tlv) {
        return *tlv + TLV_SIZEOF_HEADER;
}

int tlv_mwrite(tlv_t *tlv, void *dst) {
        memcpy(dst, *tlv, TLV_SIZEOF(tlv));
        return 0;
}

int tlv_mread(tlv_t *tlv, char *src) {

        int len = tlv_get_length(tlv);

        if (tlv_get_type(tlv) == TLV_PAD1) {
                return len;
        }
        *tlv = (tlv_t)safe_realloc(*tlv, sizeof(**tlv)
                                                * (TLV_SIZEOF_HEADER + len));
        if (*tlv == NULL) {
                ERROR("realloc", -1);
        }

        memcpy(tlv_get_value_ptr(tlv), src, len);
        return len;
}

int tlv_fwrite(tlv_t *tlv, int fd) {
        unsigned int to_write = TLV_SIZEOF(tlv);
        int status = write_all(fd, *tlv, to_write);
        if (status == -1 || (unsigned int)status != to_write) {
                ERROR("write", -1);
        }
        return 0;
}

int tlv_fread(tlv_t *tlv, int fd) {
        int len = tlv_get_length(tlv);

        *tlv = (tlv_t)safe_realloc(*tlv,
                                sizeof(**tlv) * (TLV_SIZEOF_HEADER + len));

        if (*tlv == NULL) {
                perror("realloc");
                return DZ_MEMORY_ERROR;
        }

        if (read(fd, tlv_get_value_ptr(tlv), len) < len) {
                ERROR("read", -1);
        }
        return 0;
}

int tlv_from_file(tlv_t *tlv, int fd) {

        if (read(fd, *tlv, TLV_SIZEOF_HEADER) < TLV_SIZEOF_HEADER) {
                LOGERROR("read failed");
                return -1;
        }

        if (tlv_fread(tlv, fd) == -1) {
                LOGERROR("tlv_fread failed.");
                return -1;
        }

        return 0;
}

int tlv_file2tlv(tlv_t *tlv, int fd, char type, uint32_t date) {

        int tlv_size;
        int rc = 0;
        int len = 0;

        int tlv_start = date ? TLV_SIZEOF_DATE + TLV_SIZEOF_HEADER : 0;

        tlv_size = tlv_start + TLV_SIZEOF_HEADER;

        *tlv = (tlv_t)safe_realloc(*tlv, tlv_size);

        if (*tlv == NULL) {
                ERROR("realloc", -1);
        }

        do {
                len += rc;
                if (len + tlv_start == tlv_size) {
                        tlv_size *= 2;
                        *tlv = safe_realloc(*tlv, tlv_size);
                        if (*tlv == NULL) {
                                ERROR("realloc", -1);
                        }
                }
        } while ((rc = read(fd, *tlv + tlv_start + len,
                                        tlv_size - len - tlv_start)) > 0);

        if (rc == -1) {
                PERROR("read");
                return -1;
        }

        if (len + tlv_start > TLV_MAX_SIZE) {
                LOGERROR("Too large to fit in a TLV: %d.", len + tlv_start);
        }

        (*tlv)[tlv_start] = type;
        htod(len, &((*tlv)[tlv_start + 1]));

        if (date) {
                tlv_set_type(tlv, TLV_DATED);
                tlv_set_length(tlv, len + tlv_start);
                tlv_set_date(tlv, date);
        }

        return 0;
}

int tlv_fdump(tlv_t *tlv, int fd) {
        return write_all(fd, *tlv, TLV_SIZEOF(tlv));
}

int tlv_fdump_value(tlv_t *tlv, int fd) {
        return write_all(fd, tlv_get_value_ptr(tlv), tlv_get_length(tlv));
}

const char *tlv_type2str(int tlv_type) {
        for (int i=0; tlv_types[i].name != NULL; i++) {
                if (tlv_types[i].code == tlv_type) {
                        return tlv_types[i].name;
                }
        }
        return "unknown";
}


char tlv_str2type(char *tlv_type) {
        for (int i=0; tlv_types[i].name != NULL; i++) {
                if (strcasecmp(tlv_type, tlv_types[i].name) == 0) {
                        return tlv_types[i].code;
                }
        }
        return -1;
}
