#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "utils.h"
#include "dazibao.h"

/** @file */

#ifndef BUFFLEN
/** buffer length used by various functions */
#define BUFFLEN 128
#endif

/**
 * Look for the beggining of an unbroken pad1/padN serie leading to `offset`.
 * @param d a pointer to the dazibao
 * @param offset
 * @param min_offset the minimum offset. The returned offset won't be below
 *                   this offset.
 * @return offset of the beginning of this serie on success, or the given
 *         offset if there are no pad1/padNs before it.
 * @see dz_pad_serie_end
 */
static off_t dz_pad_serie_start(dz_t *d, off_t offset, off_t min_offset);

/**
 * Skip tlv at offset, and look for the end of an unbroken pad1/padN serie
 * starting after the skipped tlv.
 * @param d a pointer to the dazibao
 * @param offset
 * @param max_offset the maximum offset to check The returned offset will never
 *        be higher than this one. If this parameter is set to 0, it won't be
 *        used. This is useful when you don't care about the maximum offset.
 * @return offset of the end of this serie on success, or the offset of the
 *         next tlv if it's not followed by pad1/padNs.
 * @see dz_pad_serie_start
 */
static off_t dz_pad_serie_end(dz_t *d, off_t offset, off_t max_offset);

int dz_create(dz_t *daz_buf, char *path) {

        int fd;
        char header[DAZIBAO_HEADER_SIZE];

        fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);

        if (fd == -1) {
                perror("open");
                if (errno == EACCES) {
                        return DZ_RIGHTS_ERROR;
                }
                return -1;
        }

        if (flock(fd, LOCK_SH) == -1) {
                close(fd);
                perror("flock");
                return -1;
        }

        header[0] = MAGIC_NUMBER;
        header[1] = 0;

        if (write(fd, header, DAZIBAO_HEADER_SIZE) < DAZIBAO_HEADER_SIZE) {
                perror("write");
                return DZ_WRITE_ERROR;
        }

        *daz_buf = fd;

        return 0;
}

int dz_open(dz_t *d, char *path, int flags) {

        int fd, lock;
        char header[DAZIBAO_HEADER_SIZE];

        fd = open(path, flags);
        if (fd == -1) {
                ERROR("open", -1);
        }

        if (flags == O_WRONLY) {
                lock = LOCK_EX;
        } else if (flags == O_RDONLY || flags == O_RDWR) {
                lock = LOCK_SH;
        } else {
                close(fd);
                return -1;
        }

        if (flock(fd, lock) == -1) {
                perror("flock");
                close(fd);
                return -1;
        }

        if (read(fd, header, DAZIBAO_HEADER_SIZE) < DAZIBAO_HEADER_SIZE) {
                perror("read");
                close(fd);
                return DZ_READ_ERROR;
        }

        if (header[0] != MAGIC_NUMBER || header[1] != 0) {
                if (close(fd) == -1) {
                        perror("close");
                }
                return DZ_WRONG_HEADER_ERROR;
        }

        *d = fd;

        return 0;
}

int dz_close(dz_t *d) {

        if (flock(*d, LOCK_UN) == -1) {
                perror("flock");
                return -1;
        }
        if (close(*d) == -1) {
                perror("close");
                return -1;
        }

        return 0;
}

off_t dz_get_size(dz_t *d) {
        if (d == NULL) {
                return (off_t)-1;
        }
        return lseek(*d, 0, SEEK_END);
}

int dz_read_tlv(dz_t *d, tlv_t *tlv, off_t offset) {
        int st;

        SAVE_OFFSET(*d);

        /* there are probably some issues to fix with large tlvs,
         * see github.com/bfontaine/Dazibao/issues/73#issuecomment-29986821
         */

        if (SET_OFFSET(*d, offset + TLV_SIZEOF_HEADER) == -1) {
                perror("lseek");
                return DZ_OFFSET_ERROR;
        }

        st = tlv_read(tlv, *d);
        RESTORE_OFFSET(*d);

        return st;
}

time_t dz_read_date_at(dz_t *d, off_t offset) {
        time_t t = (time_t)0;

        SET_OFFSET(*d, offset);
        if (read(*d, &t, TLV_SIZEOF_DATE) < 0) {
                perror("read");
                return (time_t)-1;
        }

        return t;
}

off_t dz_next_tlv(dz_t *d, tlv_t *tlv) {

        int size_read;
        off_t off_init;

        /*
         * Precondition: *tlv have to be (at least) TLV_SIZEOF_HEADER long
         */

        off_init = GET_OFFSET(*d);

        if (off_init == -1) {
                perror("lseek");
                return DZ_OFFSET_ERROR;
        }

        /* try to read regular header (TLV_SIZEOF_HEADER) */
        size_read = read(*d, *tlv, TLV_SIZEOF_HEADER);
        if (size_read == 0) {
                /* reached end of file */
                return EOD;
        } else if (size_read < 0) {
                /* read error */
                ERROR(NULL, -1);
        } else if (size_read < TLV_SIZEOF_TYPE) {
                ERROR(NULL, -1);
        } else if (tlv_get_type(*tlv) == TLV_PAD1) {
                /* we read too far, because TLV_PAD1 is only 1 byte sized */
                if (SET_OFFSET(*d, (off_init + TLV_SIZEOF_TYPE)) == -1) {
                        ERROR(NULL, -1);
                }
        } else {
                if (SET_OFFSET(*d, (off_init + TLV_SIZEOF(*tlv))) == -1) {
                        ERROR(NULL, -1);
                }
        }

        return off_init;
}

int dz_tlv_at(dz_t *d, tlv_t *tlv, off_t offset) {

        /* SAVE_OFFSET(*d); */

        if (SET_OFFSET(*d, offset) == -1) {
                ERROR("lseek", -1);
        }

        if (dz_next_tlv(d, tlv) <= 0) {
                /* RESTORE_OFFSET(*d); */
                return -1;
        }

        /* RESTORE_OFFSET(*d); */
        return 0;
}

int dz_write_tlv_at(dz_t *d, tlv_t tlv, off_t offset) {

        /* SAVE_OFFSET(*d); */

        if (SET_OFFSET(*d, offset) == -1) {
                ERROR("lseek", -1);
        }

        if (tlv_write(tlv, *d) < 0) {
                ERROR("tlv_write", -1);
        }

        /* RESTORE_OFFSET(*d); */
        return 0;
}


int dz_add_tlv(dz_t *d, tlv_t tlv) {
        off_t pad_off, eof_off;

        /* SAVE_OFFSET(*d); */

        /* find EOF offset */
        eof_off = lseek(*d, 0, SEEK_END);

        if (eof_off == 0) {
                ERROR(NULL, -1);
        }

        /* find offset of pad serie leading to EOF */
        pad_off = dz_pad_serie_start(d, eof_off, 0);

        /* set new position if needed (already is EOF offset) */
        if (pad_off != eof_off && SET_OFFSET(*d, pad_off) == -1) {
                ERROR("SET_OFFSET", -1);
        }

        /* write */
        if (dz_write_tlv_at(d, tlv, pad_off) < 0) {
                ERROR(NULL, -1);
        }

        /* truncate if needed */
        if (eof_off > pad_off + (int)TLV_SIZEOF(tlv)
                && ftruncate(*d, (pad_off + TLV_SIZEOF(tlv))) < 0 ) {
                ERROR("ftruncate", -1);
        }

        /* RESTORE_OFFSET(*d); */

        return 0;
}

off_t dz_pad_serie_start(dz_t *d, off_t offset, off_t min_offset) {
        off_t off_start, off_tmp;

        tlv_t buf = (tlv_t)malloc(sizeof(char)*TLV_SIZEOF_HEADER);

        if (buf == NULL) {
                ERROR("malloc", -1);
        }

        /* SAVE_OFFSET(*d); */

        if (SET_OFFSET(*d, MAX(min_offset, DAZIBAO_HEADER_SIZE)) == -1) {
                free(buf);
                ERROR("lseek", -1);
        }

        off_start = -1;

        while ((off_tmp = dz_next_tlv(d, &buf)) != -1
                && off_tmp != EOD
                && off_tmp < offset) {
                if (tlv_get_type(buf) == TLV_PAD1
                        || tlv_get_type(buf) == TLV_PADN) {
                        /* pad reached */
                        if (off_start == -1) {
                                off_start = off_tmp;
                        }
                } else {
                        /* tlv which is not a pad */
                        off_start = -1;
                }
        }

        if (off_tmp == -1) {
                free(buf);
                ERROR("", -1);
        }

        if(off_start == -1) {
                off_start = offset;
        }

        /* RESTORE_OFFSET(*d); */

        free(buf);
        return off_start;

}

off_t dz_pad_serie_end(dz_t *d, off_t offset, off_t max_offset) {
        off_t off_stop;
        tlv_t buf;

        if (max_offset > 0 && max_offset < offset) {
                return DZ_OFFSET_ERROR;
        }

        /* SAVE_OFFSET(*d); */

        if(SET_OFFSET(*d, offset) == -1) {
                ERROR("lseek", -1);
        }
        buf = (tlv_t)malloc(sizeof(char)*TLV_SIZEOF_HEADER);

        /* skip current tlv */
        off_stop = dz_next_tlv(d, &buf);

        /* look for the first tlv which is not a pad */
        while (off_stop != EOD && (off_stop = dz_next_tlv(d, &buf)) > 0
                        && (max_offset > 0 && off_stop < max_offset)
                        && TLV_IS_EMPTY_PAD(tlv_get_type(buf)));

        if (max_offset > 0 && off_stop > max_offset) {
                /* off_stop must be <= max_offset */
                free(buf);
                return DZ_OFFSET_ERROR;
        }
        if (off_stop == EOD) {
                off_stop = lseek(*d, 0, SEEK_END);
                if (off_stop < 0) {
                        perror("lseek");
                        free(buf);
                        return DZ_OFFSET_ERROR;
                }
        }

        /* RESTORE_OFFSET(*d); */
        free(buf);
        return off_stop;
}


int dz_rm_tlv(dz_t *d, off_t offset) {

        off_t off_start, off_end, off_eof;

        /* SAVE_OFFSET(*d); */

        off_eof = lseek(*d, 0, SEEK_END);

        if (off_eof == -1) {
                /* RESTORE_OFFSET(*d); */
                return DZ_OFFSET_ERROR;
        }

        if ((off_start = dz_pad_serie_start(d, offset, 0)) < 0) {
                return -1;
        }

        if ((off_end = dz_pad_serie_end(d, offset, 0)) < 0) {
                return -1;
        }

        /* This is commented for now because it breaks dazibaos when we
         * delete the last TLV of a compound one which is at the end of
         * the Dazibao: it truncates the file without updating the
         * length field of the compound TLV
         * See #104
        if (off_end == off_eof) { / * end of file reached * /
                if (ftruncate(*d, off_start) == -1) {
                        perror("ftruncate");
                }
                return 0;
        }
         */

        return dz_do_empty(d, off_start, off_end - off_start);
}

/**
 * Helper for dz_check_tlv_at with two additional arguments for the start and
 * the end of the search.
 * @param offset the offset of the TLV
 * @param type the type of the TLV. If this is -1, it'll won't be verified, and
 * any known TLV will work.
 * @param start the starting search offset
 * @param end the end search offset
 * @return 1 if there's such TLV, 0 if there's not, a negative number on error
 * @see dz_check_tlv_at
 **/
static int dz_limited_check_tlv_at(dz_t *d, off_t offset, int type,
                off_t start, off_t end) {

        tlv_t *t;
        off_t next = start;
        int st = 0,
            ttype;

        if (start > end || offset < start || end < offset) {
                /* no such TLV here */
                return 0;
        }

        t = (tlv_t*)malloc(sizeof(tlv_t));
        if (t == NULL || tlv_init(t) < 0) {
                free(t);
                return DZ_MEMORY_ERROR;
        }

        /* at the end of the loop, we'll have 'start < offset < next' */
        while (next <= offset && (st = dz_tlv_at(d, t, next)) == 0
                        && next <= end) {
                start = next;
                next += TLV_SIZEOF(*t);
        }
        if (st < 0) {
                tlv_destroy(t);
                return st;
        }
        if (next > end) {
                tlv_destroy(t);
                return DZ_OFFSET_ERROR;
        }
        ttype = tlv_get_type(*t);
        if (start == offset && (type < 0 || type == ttype)) {
                tlv_destroy(t);
                return 1;
        }
        if (ttype == TLV_COMPOUND) {
                start += TLV_SIZEOF_HEADER;
                tlv_destroy(t);
                return dz_limited_check_tlv_at(d, offset, type, start, next);
        }
        if (ttype == TLV_DATED) {
                start += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                tlv_destroy(t);
                return dz_limited_check_tlv_at(d, offset, type, start, next);
        }

        tlv_destroy(t);
        return 0;
}

int dz_check_tlv_at(dz_t *d, off_t offset, int type) {
        if (d == NULL) {
                return DZ_NULL_POINTER_ERROR;
        }
        if (offset < DAZIBAO_HEADER_SIZE) {
                return DZ_OFFSET_ERROR;
        }

        return dz_limited_check_tlv_at(d, offset, type, DAZIBAO_HEADER_SIZE,
                        dz_get_size(d));
}

int dz_do_empty(dz_t *d, off_t start, off_t length) {

        tlv_t buff = (tlv_t)calloc(length, sizeof(char));
        int status = 0,
            min_padn_length = TLV_SIZEOF_HEADER + 2;
        char pad1s[TLV_SIZEOF_HEADER-1];

        if (buff == NULL) {
                perror("calloc");
                return DZ_MEMORY_ERROR;
        }

        /* SAVE_OFFSET(*d); */

        if (d == NULL || start < DAZIBAO_HEADER_SIZE || length < 0) {
                status = -1;
                goto OUT;
        }

        if (length == 0) {
                goto OUT;
        }

        if (SET_OFFSET(*d, start) == -1) {
                PERROR("lseek");
                status = -1;
                goto OUT;
        }

        while (length > min_padn_length) {
                int tmp = length, st;
                if (length > TLV_MAX_SIZE) {
                        length = TLV_MAX_SIZE;
                }
                /* set type */
                tlv_set_type(&buff, TLV_PADN);
                /* set length */
                tlv_set_length(&buff, length - TLV_SIZEOF_HEADER);

                st = dz_write_tlv_at(d, buff, start);
                if (st < 0) {
                        free(buff);
                        return st;
                }

                start = start + length;
                length = tmp - length;
        }


        /* We don't have enough room to store a padN, so we fill it with
         * pad1's
         */
        if (length > 0) {
               for (int i=0; i<length; i++) {
                       pad1s[i] = TLV_PAD1;
               }
               if (write(*d, pad1s, length) < 0) {
                       PERROR("write");
               }
        }

        /* RESTORE_OFFSET(*d); */

OUT:
        free(buff);
        return status;
}

int dz_compact(dz_t *d) {

        tlv_t tlv = malloc(sizeof(*tlv)*TLV_SIZEOF_HEADER);
        off_t reading = DAZIBAO_HEADER_SIZE,
              writing = -1;

        int status = 0;

        char buff[BUFFLEN];

        if (d == NULL) {
                status = -1;
                goto OUT;
        }


        if (SET_OFFSET(*d, reading) == -1) {
                status = -1;
                goto OUT;
        }

        while ((reading = dz_next_tlv(d, &tlv)) != EOD) {
                int type = tlv_get_type(tlv);

                if ((type == TLV_PAD1) || (type == TLV_PADN)) {
                        if (writing == -1) {
                                writing = reading;
                        }
                } else{
                        if (writing != -1) {
                                if (SET_OFFSET(*d,
                                        (reading + TLV_SIZEOF_HEADER)) == -1) {

                                        PERROR("lseek");
                                        status = -1;
                                        goto OUT;
                                }

                                int len = tlv_get_length(tlv);
                                tlv_t tlv_tmp;

                                tlv_tmp = realloc(tlv,
                                sizeof(*tlv) * (TLV_SIZEOF_HEADER + len));

                                if (tlv_tmp == NULL) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }
                                tlv = tlv_tmp;

                                if (read(*d, tlv_get_value_ptr(tlv),len)
                                        < len) {
                                    PERROR("read");
                                    status = -1;
                                    goto OUT;
                                }

                                if (dz_write_tlv_at(d, tlv, writing) < 0) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }

                                writing = GET_OFFSET(*d);
                                if (writing == -1) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }

                                if (SET_OFFSET(*d, (reading +
                                            TLV_SIZEOF(tlv))) == -1) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }
                        }
                }
        }

        if (writing != -1) {
                /* this truncate is unsafe, see #104
                if (ftruncate(*d, writing) < 0) {
                        perror("ftruncate");
                        status = -1;
                }*/
        }

OUT:
        free(tlv);
        return status;
}

int dz_dump(dz_t *daz_buf, off_t end, int depth, int indent, int flag_debug) {

        tlv_t tlv = malloc(sizeof(*tlv)*TLV_SIZEOF_HEADER);
        off_t off;
        char *ind;
        if (indent > 0) {
                ind = (char*)malloc(sizeof(char)*(2*indent+1));
                memset(ind, '>', indent);
                memset(ind+indent, ' ', indent);
                ind[2*indent] = '\0';
        } else {
                ind = strdup("");
                printf("   offset |     type |   length\n");
                printf("----------+----------+---------\n");
        }

        while (((off = dz_next_tlv(daz_buf, &tlv)) != end) && (off != EOD)) {
                int tlv_type, len;
                const char *tlv_str;

                printf("%s", ind);

                tlv_type = tlv_get_type(tlv);
                len = tlv_type == TLV_PAD1 ? 0 : tlv_get_length(tlv);

                tlv_str = tlv_type2str((char) tlv_type);
                /* for option debug print pad n and pad1 only debug = 1 */
                if (((tlv_type != TLV_PADN) && (tlv_type != TLV_PAD1))
                                || flag_debug) {
                        printf("%9d | %8s | %8d\n",
                                (int)off, tlv_str, len);
                }

                switch (tlv_type) {
                        case TLV_COMPOUND:
                                if (depth > 0) {
                                        off_t current = GET_OFFSET(*daz_buf);
                                        SET_OFFSET(*daz_buf, off
                                                + TLV_SIZEOF_HEADER);
                                        if (dz_dump(daz_buf,current,
                                                (depth-1), (indent+1),
                                                flag_debug)) {
                                                free(ind);
                                                return -1;
                                        }
                                        SET_OFFSET(*daz_buf, current);
                                }
                                break;
                        case TLV_DATED:
                                if (depth > 0) {
                                        /* TODO function to print date */
                                        off_t current = GET_OFFSET(*daz_buf);
                                        SET_OFFSET(*daz_buf, off
                                        + TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE);
                                        if (dz_dump(daz_buf,current,
                                              (depth-1), (indent+1),
                                              flag_debug)) {
                                                free(ind);
                                                return -1;
                                        }
                                        SET_OFFSET(*daz_buf, current);
                                }
                                break;
                        default: break;
                }

        }
        free(ind);
        free(tlv);
        return 0;
}

#undef BUFFLEN
