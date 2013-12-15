#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include "utils.h"
#include "tlv.h"
#include "mdazibao.h"

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

        *t = (tlv_t)malloc(sizeof(**t) * TLV_SIZEOF_HEADER);
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
        return (*tlv)[0];
}

void tlv_set_type(tlv_t *tlv, char t) {
        (*tlv)[0] = t;
}

void tlv_set_length(tlv_t *tlv, unsigned int n) {
        htod(n, tlv_get_length_ptr(tlv));
}

char *tlv_get_length_ptr(tlv_t *tlv) {
        return *tlv + TLV_SIZEOF_TYPE;
}

unsigned int tlv_get_length(tlv_t *tlv) {
        return tlv_get_type(tlv) == TLV_PAD1 ?
                0 : dtoh(tlv_get_length_ptr(tlv));
}

char *tlv_get_value_ptr(tlv_t *tlv) {
        return *tlv + TLV_SIZEOF_HEADER;
}

int tlv_mwrite(tlv_t *tlv, void *dst) {
        memcpy(dst, *tlv, TLV_SIZEOF(tlv));
        return 0;
}

int tlv_mread(tlv_t *tlv, void *src) {

        int len = tlv_get_length(tlv);

        if (tlv_get_type(tlv) == TLV_PAD1
                || tlv_get_type(tlv) == TLV_PADN ) {
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

int tlv_fwrite(tlv_t tlv, int fd) {
/*
        unsigned int to_write = TLV_SIZEOF(tlv);
        int status = write_all(fd, tlv, to_write);
        if (status == -1 || (unsigned int)status != to_write) {
                ERROR("write", -1);
        }
*/
        return 0;
}

int tlv_fread(tlv_t *tlv, int fd) {
/*
        int len = tlv_get_length(*tlv);

        *tlv = (tlv_t)safe_realloc(*tlv, sizeof(char)
                                                * (TLV_SIZEOF_HEADER + len));

        if (*tlv == NULL) {
                perror("realloc");
                return DZ_MEMORY_ERROR;
        }

        if (read(fd, tlv_get_value_ptr(*tlv), len) < len) {
                ERROR("read", -1);
        }
*/
        return 0;
}


int tlv_fdump(tlv_t tlv, int fd) {
/*
        return write(fd, tlv, TLV_SIZEOF(tlv));
*/
        return 0;
}

int tlv_fdump_value(tlv_t tlv, int fd) {
/*
  return write(fd, tlv_get_value_ptr(tlv), tlv_get_length(tlv));
*/
        return 0;
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

int tlv_create_compound(tlv_t *tlv_c, tlv_t *value, int buff_size) {
        unsigned int tlv_size;
        if (tlv_init(tlv_c) < 0) {
                printf("[tlv_create_compound] error to init tlv");
                return -1;
        }
        tlv_set_type(tlv_c, (char) TLV_COMPOUND);
        tlv_set_length(tlv_c, buff_size);
        tlv_size = tlv_mread(tlv_c, value);
        return tlv_size;
}

int tlv_create_date(tlv_t *tlv_d, tlv_t *value_tlv, int value_size) {
        unsigned int tlv_size ;
        int real_time = htonl(time(NULL));

        tlv_size = TLV_SIZEOF_DATE + value_size;
        *tlv_d = malloc((TLV_SIZEOF_HEADER + tlv_size) * sizeof(tlv_t));
        if (*tlv_d == NULL) {
                printf("[tlv_create_date] error to init tlv");
                return -1;
        }
        tlv_set_type(tlv_d, (char) TLV_DATED );
        tlv_set_length(tlv_d, tlv_size);
        char *t = tlv_get_value_ptr(tlv_d);
        memcpy(t, &real_time, TLV_SIZEOF_DATE);
        memcpy(t + TLV_SIZEOF_DATE * sizeof(tlv_t), *value_tlv, value_size);
        return tlv_size;
}

int tlv_create_path(char *path, tlv_t *tlv, char *type) {
        tlv_t buff;
        int tlv_size, fd;
        struct stat st_path;

        fd = open(path, O_RDONLY);
        if (fd < 0) {
                printf("[tlv_create_path] error open");
                return -1;
        }
        if (fstat(fd, &st_path)) {
                printf("[tlv_create_path] error stat");
                return -1;
        }

        buff = mmap(NULL, st_path.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (buff == MAP_FAILED) {
                printf("[tlv_create_path] error mmap");
                return -1;
        }

        tlv_set_type(tlv, type);
        tlv_set_length(tlv, st_path.st_size);
        tlv_size = tlv_mread(tlv, buff);
        close(fd);
        return tlv_size;
}

int tlv_create_input(tlv_t *tlv, char *type) {
        char reader[BUFFSIZE],
             *buff = NULL;
        unsigned int buff_size = 0;
        int read_size;

        while((read_size = read(STDIN_FILENO, reader, BUFFSIZE)) > 0) {

                buff_size += read_size;

                if(buff_size > TLV_MAX_VALUE_SIZE) {
                        fprintf(stderr, "tlv too large\n");
                        exit(EXIT_FAILURE);
                }

                buff = safe_realloc(buff, sizeof(*buff) * buff_size);

                if(buff == NULL) {
                        perror("realloc");
                        return DZ_MEMORY_ERROR;
                }

                memcpy(buff + (buff_size - read_size), reader, read_size);
        }

        tlv_set_type(tlv, type);
        tlv_set_length(tlv, st_path.st_size);
        tlv_size = tlv_mread(tlv, buff);
        free(buff);
        return tlv_size;
}


