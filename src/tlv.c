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
#include "logging.h"

/** @file */
/** buffer size used in various functions */
#define BUFFSIZE 512
/*
const char *PNG_SIGNATURE = "\211PNG\r\n\032\n";
const char *JPG_SIGNATURE = "\255\216\255";
const char *GIF_SIGNATURE = "GIF";
*/

/**
 * All recognized TLV types.
 **/
struct tlv_type tlv_types[] = {
        { TLV_PAD1     , "pad1"     },
        { TLV_PADN     , "padN"     },
        { TLV_TEXT     , "text"     },
        { TLV_PNG      , "png"      },
        { TLV_JPEG     , "jpg"      },
        { TLV_COMPOUND , "compound" },
        { TLV_DATED    , "dated"    },
        { TLV_GIF      , "gif"      },
        { TLV_TIFF     , "tiff"     },
        { TLV_MP3      , "mp3"      },
        { TLV_MP4      , "mp4"      },
        { TLV_BMP      , "bmp"      },
        { TLV_OGG      , "ogg"      },
        { TLV_MIDI     , "mid"      },
        { -1           , NULL       }
};

/**
 * "Associative array" of TLV type / file signature
 **/
struct type_signature sigs[] =  {
        { TLV_PNG,  "\211PNG\r\n\032\n" },
        { TLV_JPEG, "\255\216\255"      },
        { TLV_GIF,  "GIF87a"            },
        { TLV_GIF,  "GIF89a"            },
        { TLV_TIFF, "\073\073\042\000"  },
        { TLV_TIFF, "\077\077\000\042"  },
        { TLV_MP3,  "\255\251"          },
        { TLV_MP3,  "\073\068\051"      },
        { TLV_MP4,  "\051\103\112\053"  },
        { TLV_BMP,  "\066\077"  },
        { TLV_OGG,  "\079\103\103\083"  },
        { TLV_MIDI, "\077\084\104\100"  }
};


unsigned char guess_type(char *src, unsigned int len) {

        unsigned int i;
        for (i = 0; i < sizeof(sigs)/sizeof(*sigs); i++) {
                if (len < strlen(sigs[i].signature)) {
                        continue;
                }
                if (strncmp(sigs[i].signature, src,
                                strlen(sigs[i].signature)) == 0) {
                        return sigs[i].type;
                }
        }

        /* TODO: Check for type text. */
        return TLV_TEXT;
}

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

int tlv_import_from_file(tlv_t *tlv, int fd) {

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

int tlv_from_file(tlv_t *tlv, int fd, char type, uint32_t date) {

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
        if (tlv_type == NULL) {
                return -1;
        }
        for (int i=0; tlv_types[i].name != NULL; i++) {
                if (strcasecmp(tlv_type, tlv_types[i].name) == 0) {
                        return tlv_types[i].code;
                }
        }
        return -1;
}

int tlv_create_compound(tlv_t *tlv_c, tlv_t *value, int buff_size) {
        tlv_set_type(tlv_c, (unsigned char) TLV_COMPOUND);
        tlv_set_length(tlv_c, buff_size);
        tlv_mread(tlv_c, *value);
        return TLV_SIZEOF(tlv_c);
}

int tlv_create_date(tlv_t *tlv_d, tlv_t *value_tlv, int value_size) {
        unsigned int tlv_size ;
        int real_time = htonl(time(NULL));
        char *buff;

        tlv_size = TLV_SIZEOF_DATE + value_size;
        buff = (char *)malloc(sizeof(*buff)* tlv_size);
        memcpy(buff, &real_time, TLV_SIZEOF_DATE);
        memcpy(buff + TLV_SIZEOF_DATE, *value_tlv, value_size);

        tlv_set_type(tlv_d, (unsigned char) TLV_DATED );
        tlv_set_length(tlv_d, tlv_size);
        tlv_mread(tlv_d, buff);
        free(buff);
        return TLV_SIZEOF(tlv_d);
}

int tlv_create_path(char *path, tlv_t *tlv, char *type) {
        tlv_t buff;
        int fd;
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

        tlv_set_type(tlv,(unsigned char)  *type);
        tlv_set_length(tlv, st_path.st_size);
        tlv_mread(tlv, buff);
        close(fd);
        return TLV_SIZEOF(tlv);
}

int tlv_create_input(tlv_t *tlv, char *type) {
        char reader[BUFFSIZE],
             *buff = NULL;
        unsigned int buff_size = 0;
        int read_size;


        while ((read_size = read(STDIN_FILENO, reader, BUFFSIZE)) > 0) {

                buff_size += read_size;

                if (buff_size > TLV_MAX_VALUE_SIZE) {
                        fprintf(stderr, "tlv too large\n");
                        exit(EXIT_FAILURE);
                }

                buff = safe_realloc(buff, sizeof(*buff) * buff_size);

                if (buff == NULL) {
                        perror("realloc");
                        return DZ_MEMORY_ERROR;
                }

                memcpy(buff + (buff_size - read_size), reader, read_size);
        }

        tlv_set_type(tlv, (unsigned char) *type);
        tlv_set_length(tlv, buff_size);
        tlv_mread(tlv, buff);
        free(buff);
        return TLV_SIZEOF(tlv);
}


