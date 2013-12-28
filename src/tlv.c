#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
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
        { TLV_PDF      , "pdf"      },
        { TLV_LONGH    , "long_tlv" },
        { -1           , NULL       }
};

/**
 * Associations of TLV types and their file signature
 **/
struct type_signature sigs[] =  {
        { TLV_BMP  , "BM"                               , 2 },
        { TLV_GIF  , "GIF87a"                           , 6 },
        { TLV_GIF  , "GIF89a"                           , 6 },
        { TLV_JPEG , "\xFF\xD8\xFF"                     , 3 },
        { TLV_MIDI , "MThd"                             , 4 },
        { TLV_MP3  , "ID3"                              , 3 },
        { TLV_MP3  , "\255\251"                         , 2 },
        { TLV_MP4  , "\x00\x00\x00\x14\x66\x74\x79\x70" , 8 },
        { TLV_MP4  , "\x00\x00\x00\x20\x66\x74\x79\x70" , 8 },
        { TLV_MP4  , "\x33\x67\x70\x35"                 , 4 },
        { TLV_OGG  , "OggS"                             , 4 },
        { TLV_PDF  , "%PDF"                             , 4 },
        { TLV_PNG  , "\211PNG\r\n\032\n"                , 8 },
        { TLV_TEXT , "\xEF\xBB\xBF"                     , 3 }, /* UTF-8 BOM */
        { TLV_TIFF , "\x49\x49\x2A\x00"                 , 4 },
        { TLV_TIFF , "\x4D\x4D\x00\x2A"                 , 4 }
};

/**
 * Helper for making long tlv
 */
static void mk_longh(tlv_t *tlv, int type, uint32_t len);

/**
 * Helper for tlv_guess_type. Test for a <= b <= c, using unsigned chars.
 * @param a
 * @param b
 * @param c
 **/
static inline char between(char a, char b, char c) {
        return (unsigned char)a <= (unsigned char)b
                        && (unsigned char)b <= (unsigned char)c;
}

unsigned char tlv_guess_type(char *src, unsigned int len) {

        unsigned int i;
        char is_utf8;
        const char ctrl_chars[] = "\t\n\v\r"; /* acceptable ctrl chars */

        if (len <= 3) {
                /* the type cannot be determined from less than 3 or 4 chars */
                return (unsigned char)-1;
        }

        for (i = 0; i < sizeof(sigs)/sizeof(*sigs); i++) {
                unsigned int comp_len = MIN(len, sigs[i].len);

                if (memcmp(src, sigs[i].signature, comp_len) == 0) {
                        return sigs[i].type;
                }
        }

        /* check for UTF8 */
        is_utf8 = 1;
        for (i = 0; i<len && is_utf8; i++) {
                unsigned char c = src[i];

                /* control chars */
                if (c < 0x20) {
                        if (strchr(ctrl_chars, c) == NULL) {
                                is_utf8 = 0;
                                break;
                        }
                        continue;
                }

                /* see www.unicode.org/versions/Unicode6.2.0/ch03.pdf
                 * table 3-7 p.95
                 *
                 * Bytes: 1       2       3       4
                 *        00..7F
                 *        C2..DF  80..BF
                 *        E0      A0..BF  80..BF
                 *        E1..EC  80..BF  80..BF
                 *        ED      80..9F  80..BF
                 *        EE..EF  80..BF  80..BF
                 *        F0      90..BF  80..BF  80..BF
                 *        F1..F3  80..BF  80..BF  80..BF
                 *        F4      80..8F  80..BF  80..BF
                 */
                /* 00..7F */
                if (c <= 0x7F) {
                        continue;
                }
                /* C0, C1, F4..FF */
                if (c >= 0xF5 || c == 0xC0 || c == 0xC1) {
                        is_utf8 = 0;
                        break;
                }

                /* first byte is C2..DF */
                if (between(0xC2, c, 0xDF)) {
                        if (i+1 < len && !between(0x80, src[i+1], 0xBF)) {
                                is_utf8 = 0;
                                break;
                        }
                        i += 1;
                }

                /* first byte is E0, E1..EC, ED or EE..EF */
                if (between(0xE0, c, 0xEF)) {
                        if (i+2 < len && !between(0x80, src[i+2], 0xBF)) {
                                is_utf8 = 0;
                                break;
                        }

                        if (i+1 >= len) {
                                i += 2;
                                continue;
                        }

                        /* EO A0..BF 80..BF */
                        if (c == 0xE0 && !between(0xA0, src[i+1], 0xBF)) {
                                is_utf8 = 0;
                                break;
                        }

                        /* first byte is E1..EC or EE..EF */
                        if (between(0xE1, c, 0xEF) && c != 0xED
                                        && !between(0x80, src[i+1], 0xBF)) {
                                is_utf8 = 0;
                                break;
                        }

                        /* ED 90..9F 80..BF */
                        if (c == 0xED && !between(0x90, src[i+1], 0x9F)) {
                                is_utf8 = 0;
                                break;
                        }

                        i += 2;
                }

                /* first byte is F0, F1..F3 or F4 */
                if (between(0xF0, c, 0xF4)) {
                        if (   (i+2 < len && !between(0x80, src[i+2], 0xBF))
                            || (i+3 < len && !between(0x80, src[i+3], 0xBF))) {
                                is_utf8 = 0;
                                break;
                        }

                        if (i+1 >= len) {
                                i+= 3;
                                continue;
                        }

                        /* first byte is F0 */
                        if (c == 0xF0 && !between(0x90, src[i+1], 0xBF)) {
                                is_utf8 = 0;
                                break;
                        }

                        if (!between(0x80, src[i+1], 0xBF)) {
                                is_utf8 = 0;
                                break;
                        }

                        i += 3;
                }
        }
        if (is_utf8) {
                return TLV_TEXT;
        }

        return (unsigned char)-1;
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

void tlv_mdump(tlv_t *tlv, char *dst) {
        memcpy(dst, *tlv, TLV_SIZEOF(tlv));
}

void tlv_mdump_value(tlv_t *tlv, char *dst) {
        memcpy(dst, tlv_get_value_ptr(tlv), tlv_get_length(tlv));
}

const char *tlv_type2str(int tlv_type) {
        for (int i=0; tlv_types[i].name != NULL; i++) {
                if ((char)tlv_types[i].code == (char)tlv_type) {
                        return tlv_types[i].name;
                }
        }
        return NULL;
}

int tlv_str2type(char *tlv_type) {
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
        uint32_t timestamp;
        char *buff;

        timestamp = (uint32_t) time(NULL);
        timestamp = htonl(timestamp);

        tlv_size = TLV_SIZEOF_DATE + value_size;
        buff = (char *)malloc(sizeof(*buff)* tlv_size);
        memcpy(buff, &timestamp, TLV_SIZEOF_DATE);
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


int ltlv_nb_chunks(size_t size) {
        return size / TLV_MAX_VALUE_SIZE
                + MIN(1, size % TLV_MAX_VALUE_SIZE);
}

size_t ltlv_mk_tlv(tlv_t *tlv, char *src, int type, int len) {
        int
                nb_chunks = ltlv_nb_chunks(len),
                be_len = htonl(len),
                remaining = len,
                w_idx = 0,
                r_idx = 0;

        if (*tlv == NULL) {
                return -1;
        }

        /* Set TLV_LONGH header */
        tlv_set_type(tlv, TLV_LONGH);
        tlv_set_length(tlv, TLV_SIZEOF_TYPE + sizeof(int));
        (*tlv)[TLV_SIZEOF_HEADER] = type;
        memcpy(&((*tlv)[TLV_SIZEOF_HEADER + 1]), &be_len, sizeof(uint32_t));

        w_idx = TLV_SIZEOF(tlv);

        for (int i = 0; i < nb_chunks; i++) {
                int size = MIN(TLV_MAX_VALUE_SIZE, remaining);
                tlv_t false_tlv = *tlv + w_idx;
                (*tlv)[w_idx] = TLV_LONGC;
                tlv_set_length(&false_tlv, size);
                w_idx += TLV_SIZEOF_HEADER;
                memcpy(&((*tlv)[w_idx]), src + r_idx, size);
                r_idx += size;
                w_idx += size;
                remaining -= size;
        }

        return w_idx;
}

size_t ltlv_mwrite(tlv_t *tlv, char *dst) {
        size_t len = ltlv_get_total_length(tlv);
        memcpy(dst, *tlv, len);
        return len;
}

size_t ltlv_fwrite(tlv_t *tlv, int fd) {
        size_t len = ltlv_get_total_length(tlv);
        return write_all(fd, *tlv, len);
}

uint32_t ltlv_real_data_length(tlv_t *tlv) {
        return ntohl(*((uint32_t *)((*tlv) + TLV_SIZEOF_HEADER + 1)));
}

int ltlv_real_data_type(tlv_t *tlv) {
        return (*tlv)[TLV_SIZEOF_HEADER];
}

size_t ltlv_get_total_length(tlv_t *tlv) {

        size_t len;

        memcpy(&len, &((*tlv)[TLV_SIZEOF_HEADER + 1]), sizeof(uint32_t));
        len = ntohl(len);

        return len
                + ltlv_nb_chunks(len) * TLV_SIZEOF_HEADER
                + TLV_SIZEOF_LONGH;
}

int tlv_from_inputs(tlv_t *tlv, struct tlv_input *inputs, int nb_inputs,
                time_t date) {

        int
                i,
                len = 0,
                cmpnd_idx = 0,
                write_idx,
                status = 0,
                nb_lhead = 0,
                nb_lcontent = 0,
                content_idx = 0;

        /* Compute needed length */

        for (int i = 0; i < nb_inputs; i++) {
                if (inputs[i].len > TLV_MAX_VALUE_SIZE) {
                        nb_lhead++;
                        nb_lcontent += ltlv_nb_chunks(inputs[i].len);
                } else {
                        len += TLV_SIZEOF_HEADER;
                }

                len += inputs[i].len;
        }

        if (nb_inputs > 1) {
                /* If more than one input
                 * we make a compound */
                len += TLV_SIZEOF_HEADER;
                content_idx += TLV_SIZEOF_HEADER;
        }

        /* Set date and update pointers if needed. */
        if (date) {
                cmpnd_idx += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                content_idx += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                len += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
        }

        /* Add space needed by large tlv headers */
        len += nb_lcontent * TLV_SIZEOF_HEADER;
        len += nb_lhead * TLV_SIZEOF_LONGH;

        /* If we are making a compound with large content
         * we will need to include it in a large tlv
         * But we wont need space for compound header
         * as it will be included in LONGH value
         * content_idx will move TLV_SIZEOF_LONGH */

        if (len > TLV_MAX_SIZE && nb_inputs > 1) {
                nb_lhead++;
                len -= TLV_SIZEOF_HEADER;
                len += ltlv_nb_chunks(len) * TLV_SIZEOF_HEADER;
                len += TLV_SIZEOF_LONGH;
                content_idx += TLV_SIZEOF_LONGH - TLV_SIZEOF_HEADER;
        }

        /* Same behavior with a date,
         * but cmpnd_idx move as well */
        if (len > TLV_MAX_SIZE && date) {
                nb_lhead++;
                len -= TLV_SIZEOF_HEADER;
                len += ltlv_nb_chunks(len) * TLV_SIZEOF_HEADER;
                len += TLV_SIZEOF_LONGH;
                content_idx += TLV_SIZEOF_LONGH - TLV_SIZEOF_HEADER;
                cmpnd_idx += TLV_SIZEOF_LONGH - TLV_SIZEOF_HEADER;
        }

        *tlv = safe_realloc(*tlv, sizeof(**tlv) * len);

        if (*tlv == NULL) {
                PERROR("safe_realloc");
                return -1;
        }

        write_idx = content_idx;

        /* Copy inputs value and add TLV headers */

        for (i = 0; i < nb_inputs; i++) {

                if (inputs[i].len > TLV_MAX_VALUE_SIZE) {

                        char *false_tlv = *tlv + write_idx;

                        /* Need to insert TLV in a long TLV */
                        int wrote = ltlv_mk_tlv(
                                &false_tlv,
                                inputs[i].data,
                                inputs[i].type,
                                inputs[i].len);
                        write_idx += wrote;
                } else {
                        (*tlv)[write_idx] = inputs[i].type;
                        write_idx += TLV_SIZEOF_TYPE;
                        htod(inputs[i].len, *tlv + write_idx);
                        write_idx += TLV_SIZEOF_LENGTH;
                        memcpy(*tlv + write_idx,
                                inputs[i].data,
                                inputs[i].len);
                        write_idx += inputs[i].len;
                }
        }

        /* Write compound header if needed */

        if (nb_inputs > 1) {
                int content_len = write_idx - content_idx;
                char *false_tlv = *tlv + cmpnd_idx;
                if (content_len > TLV_MAX_VALUE_SIZE) {
                        mk_longh(&false_tlv, TLV_COMPOUND, content_len);
                        uint32_t new_size = ltlv_split_value(
                                false_tlv + TLV_SIZEOF_LONGH,
                                content_len);
                        write_idx = content_idx + new_size;
                } else {
                        tlv_set_type(&false_tlv, TLV_COMPOUND);
                        tlv_set_length(&false_tlv, content_len);
                }
        }

        if (date != 0) {
                int content_len = write_idx - (cmpnd_idx - TLV_SIZEOF_DATE);
                if (content_len > TLV_MAX_VALUE_SIZE) {
                        mk_longh(tlv, TLV_DATED, content_len);
                        memcpy(*tlv + TLV_SIZEOF_LONGH, &date,
                                TLV_SIZEOF_DATE);
                        uint32_t new_size = ltlv_split_value(
                                *tlv + TLV_SIZEOF_LONGH,
                                content_len);
                        write_idx = TLV_SIZEOF_LONGH + new_size;
                } else {
                        tlv_set_type(tlv, TLV_DATED);
                        tlv_set_length(tlv, content_len);
                        tlv_set_date(tlv, date);
                }
        }

        if (write_idx != len) {
                LOGERROR("written %d bytes, expected %d.", write_idx, len);
                status = -1;
        }

        return status;

}

static void mk_longh(tlv_t *tlv, int type, uint32_t len) {
        uint32_t be_len = htonl(len);
        tlv_set_type(tlv, TLV_LONGH);
        tlv_set_length(tlv, TLV_SIZEOF_TYPE + sizeof(uint32_t));
        *(tlv_get_value_ptr(tlv)) = type;
        memcpy(tlv_get_value_ptr(tlv) + 1, &be_len, sizeof(be_len));
}

/**
 * Split src in multiple LONGC tlvs
 * src MUST be large enough to contain
 * the result
 * @return New size of tlv list
 */

uint32_t ltlv_split_value(char *src, uint32_t len) {

        int
                nb_chunks = ltlv_nb_chunks(len),
                w_idx = 0;
        uint32_t remaining = len;

        for (int i = 0; i < nb_chunks; i++) {
                uint32_t size = MIN(TLV_MAX_VALUE_SIZE, remaining);
                /* shift memory */
                memmove(src + w_idx + TLV_SIZEOF_HEADER,
                        src + w_idx,
                        remaining);
                /* write LONGC header */
                src[w_idx] = TLV_LONGC;
                htod(size, src + w_idx + 1);
                /* update counters */
                w_idx += size + TLV_SIZEOF_HEADER;
                remaining -= size;
        }

        return w_idx;
}
