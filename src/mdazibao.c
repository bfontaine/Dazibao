#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include "mdazibao.h"
#include "utils.h"
#include "tlv.h"

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

int sync_file(dz_t *d) {
        if(ftruncate(d->fd, d->len) == -1) {
                PERROR("ftruncate");
                return -1;
        }

        if (msync(d->data, d->len, MS_SYNC) == -1) {
                return -1;
        }
        return 0;
}

int dz_mmap_data(dz_t *d, size_t t) {

        size_t page_size, real_size;
        int prot;

        page_size = (size_t) sysconf(_SC_PAGESIZE);
        real_size = (size_t) page_size
                * (t/page_size + (t % page_size == 0 ? 0 : 1));

        if(d->fflags == O_RDWR && ftruncate(d->fd, real_size) == -1) {
                PERROR("ftruncate");
                return -1;
        }

        prot = PROT_READ | (d->fflags == O_RDWR ? PROT_WRITE : 0);

        d->data = (char *)mmap(NULL, real_size, prot, MAP_SHARED, d->fd, 0);

        if (d->data == MAP_FAILED) {
                PERROR("mmap");
                return -1;
        }

        d->space = real_size;
        return 0;
}

int dz_remap(dz_t *d, size_t t) {

        if (d->fflags == O_RDONLY) {
                return -1;
        }

        if (sync_file(d) == -1) {
                return -1;
        }

        if (munmap(d->data, d->len) == -1) {
                return -1;
        }

        if (dz_mmap_data(d, t) == -1) {
                return -1;
        }

        return 0;
}

int dz_create(dz_t *d, char *path) {

        char header[DAZIBAO_HEADER_SIZE];

        d->fd = open(path, O_CREAT | O_EXCL | O_RDWR, 0644);

        if (d->fd == -1) {
                ERROR("open", -1);
        }

        if (flock(d->fd, LOCK_EX) == -1) {
                PERROR("flock");
                goto PANIC;
        }

        d->fflags = O_RDWR;

        if (dz_mmap_data(d, DAZIBAO_HEADER_SIZE)) {
                goto PANIC;
        }

        d->data[0] = MAGIC_NUMBER;
        d->data[1] = 0;
        d->len = DAZIBAO_HEADER_SIZE;

        goto OUT;
PANIC:
        if (close(d->fd) == -1) {
                PERROR("close");
        }
        return -1;

OUT:
        if (sync_file(d) == -1) {
                return -1;
        }
        return 0;
}

int dz_open(dz_t *d, char *path, int flags) {

        int lock;
        char header[DAZIBAO_HEADER_SIZE];
        struct stat st;

        if (flags == O_RDWR) {
                lock = LOCK_EX;
        } else if (flags == O_RDONLY) {
                lock = LOCK_SH;
        } else {
                return -1;
        }

        d->fd = open(path, flags);

        if (d->fd == -1) {
                return -1;
        }

        if (flock(d->fd, lock) == -1) {
                PERROR("flock");
                goto PANIC;
        }

        if (fstat(d->fd, &st) < 0) {
                PERROR("fstat");
                goto PANIC;
        }

        if (st.st_size < DAZIBAO_HEADER_SIZE) {
                goto PANIC;
        }

        d->offset = DAZIBAO_HEADER_SIZE;
        d->len = st.st_size;
        d->fflags = flags;

        if (dz_mmap_data(d, d->len) == -1) {
                goto PANIC;
        }

        if (d->data[0] != MAGIC_NUMBER || d->data[1] != 0) {
                goto PANIC;
        }

        goto OUT;
PANIC:
        if (close(d->fd) == -1) {
                PERROR("close");
        }
        return -1;
OUT:
        return 0;

}

int dz_close(dz_t *d) {

        if (d->fflags == O_RDWR && sync_file(d) == -1) {
                return -1;
        }

        if (flock(d->fd, LOCK_UN) == -1) {
                perror("flock");
                return -1;
        }

        if (close(d->fd) == -1) {
                PERROR("close");
                return -1;
        }

        return 0;

}

int dz_read_tlv(dz_t *d, tlv_t *tlv, off_t offset) {
        return tlv_mread(tlv, d->data);
}

time_t dz_read_date_at(dz_t *d, off_t offset) {
        time_t t = 0;
        memcpy(&t, d->data + offset, TLV_SIZEOF_DATE);
        return t;
}

off_t dz_next_tlv(dz_t *d, tlv_t *tlv) {
        int rc;
        int off_init = d->offset;

        if (d->len == d->offset) {
                return EOD;
        } else if (d->len < d->offset + TLV_SIZEOF_TYPE) {
                return -1;
        } else {
                tlv_set_type(tlv, d->data[d->offset]);
                d->offset += TLV_SIZEOF_TYPE;
        }

        if (tlv_get_type(tlv) == TLV_PAD1) {
                return off_init;
        }

        if (d->len < d->offset + TLV_SIZEOF_LENGTH) {
                return -1;
        }

        memcpy(tlv_get_length_ptr(tlv), d->data + d->offset,
                        TLV_SIZEOF_LENGTH);

        d->offset += TLV_SIZEOF_LENGTH;

        rc = tlv_mread(tlv, d->data + d->offset);

        if (rc < 0) {
                return -1;
        }

        d->offset += rc;
        return off_init;
}

int dz_tlv_at(dz_t *d, tlv_t *tlv, off_t offset) {
        dz_t tmp = {0, d->fflags, d->len - offset, 0, d->space - offset,
                d->data + offset};
        return dz_next_tlv(&tmp, tlv);
}

int dz_write_tlv_at(dz_t *d, tlv_t *tlv, off_t offset) {
        return tlv_mwrite(tlv, d->data + offset);
}

int dz_add_tlv(dz_t *d, tlv_t *tlv) {

        off_t pad_off, eof_off;

        eof_off = d->len;

        /* find offset of pad serie leading to EOF */
        pad_off = dz_pad_serie_start(d, eof_off, 0);

        if ((d->len - pad_off) < (unsigned int)TLV_SIZEOF(tlv)) {
                if (dz_remap(d, d->len + (TLV_SIZEOF(tlv) - pad_off)) == -1) {
                        return -1;
                }
        }

        d->len = pad_off + TLV_SIZEOF(tlv);

        /* write */
        if (dz_write_tlv_at(d, tlv, pad_off) < 0) {
                ERROR(NULL, -1);
        }

        if (sync_file(d) == -1) {
                return -1;
        }
        return 0;
}

static off_t dz_pad_serie_start(dz_t *d, off_t offset, off_t min_offset) {

        off_t off_start, off_tmp;
        tlv_t buf;

        dz_t tmp = {0, d->fflags, offset, DAZIBAO_HEADER_SIZE,
                d->space, d->data};

        tlv_init(&buf);

        off_start = -1;

        while ((off_tmp = dz_next_tlv(&tmp, &buf)) != -1
                && off_tmp != EOD
                && off_tmp < offset) {
                if (tlv_get_type(&buf) == TLV_PAD1
                        || tlv_get_type(&buf) == TLV_PADN) {
                        /* pad reached */
                        if (off_start == -1) {
                                off_start = off_tmp;
                        }
                } else {
                        /* tlv which is not a pad */
                        off_start = -1;
                }
        }

        if (off_tmp == -1 || off_start == -1) {
                off_start = offset;
        }

        tlv_destroy(&buf);
        return off_start;
}

static off_t dz_pad_serie_end(dz_t *d, off_t offset, off_t max_offset) {
        off_t off_stop;
        tlv_t buf;

        if (max_offset > 0 && max_offset < offset) {
                return DZ_OFFSET_ERROR;
        }

        tlv_init(&buf);
        d->offset = offset;

        /* skip current tlv */
        off_stop = dz_next_tlv(d, &buf);

        /* look for the first tlv which is not a pad */
        while (off_stop != EOD && (off_stop = dz_next_tlv(d, &buf)) > 0
                        && (max_offset > 0 && off_stop < max_offset)
                        && TLV_IS_EMPTY_PAD(tlv_get_type(&buf)));

        if (max_offset > 0 && off_stop > max_offset) {
                /* off_stop must be <= max_offset */
                tlv_destroy(&buf);
                return DZ_OFFSET_ERROR;
        }
        if (off_stop == EOD) {
                off_stop = d->len;
        }

        tlv_destroy(&buf);
        return off_stop;
}


int dz_rm_tlv(dz_t *d, off_t offset) {
        off_t off_start_parent = 0,
              off_end_parent = 0,
              off_parent,
              off_start,
              off_end,
              off_eof;
        off_t *parents = NULL;

        /* SAVE_OFFSET(*d); */

        if (dz_check_tlv_at(d, offset, -1, &parents) <= 0
                        || parents[0] != offset) {
                free(parents);
                return DZ_OFFSET_ERROR;
        }

        off_eof = d->len;

        if (off_eof == -1) {
                /* RESTORE_OFFSET(*d); */
                free(parents);
                return DZ_OFFSET_ERROR;
        }

        off_parent = parents[1];
        free(parents);

        if (off_parent != 0) {
                /* the rm-ed TLV is a child one */
                tlv_t t;
                int type, st;
                st = tlv_init(&t);

                if (st < 0) {
                        tlv_destroy(&t);
                        return st;
                }

                if ((st = dz_tlv_at(d, &t, off_parent)) < 0) {
                        tlv_destroy(&t);
                        return st;
                }

                type = tlv_get_type(&t);
                off_start_parent = off_parent + TLV_SIZEOF_HEADER;
                off_end_parent = off_parent + TLV_SIZEOF(&t);

                if (type == TLV_DATED) {
                        off_start_parent += TLV_SIZEOF_DATE;
                } else if (type != TLV_COMPOUND) {
                        tlv_destroy(&t);
                        return DZ_TLV_TYPE_ERROR;
                }

                tlv_destroy(&t);
        }

        off_start = dz_pad_serie_start(d, offset, off_start_parent);
        if (off_start < 0) {
                return -1;
        }

        if ((off_end = dz_pad_serie_end(d, offset, off_end_parent)) < 0) {
                return -1;
        }

        if (off_end == off_eof && off_parent == 0) {
                /* truncate the end of the file if the rm-ed TLV is a top-level
                 * one */
                d->len = off_start;
        }

        return dz_do_empty(d, off_start, off_end - off_start);
}

/**
 * Helper for dz_limited_check_tlv_at. Takes an array of TLV_MAX_DEPTH offsets
 * and add an element at its end if there's enough room.
 **/
static void push_offset(off_t *arr, off_t off) {
        int i=0;
        if (arr == NULL) {
                return;
        }
        for (; i<TLV_MAX_DEPTH && arr[i] != (off_t)0; i++);
        if (i == TLV_MAX_DEPTH) {
                return;
        }
        arr[i] = off;
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
                off_t start, off_t end, off_t *parents) {

        tlv_t t;
        off_t next = start;
        int st = 0,
            ttype;

        if (start > end || offset < start || end < offset) {
                /* no such TLV here */
                return 0;
        }

        tlv_init(&t);

        /* at the end of the loop, we'll have 'start < offset < next' */
        while (next <= offset && (st = dz_tlv_at(d, &t, next)) == 0
                        && next <= end) {
                start = next;
                next += TLV_SIZEOF(&t);
        }
        if (st < 0) {
                tlv_destroy(&t);
                return st;
        }
        if (next > end) {
                tlv_destroy(&t);
                return DZ_OFFSET_ERROR;
        }
        ttype = tlv_get_type(&t);
        if (start == offset && (type < 0 || type == ttype)) {
                tlv_destroy(&t);
                push_offset(parents, offset);
                return 1;
        }
        if (ttype == TLV_COMPOUND) {
                tlv_destroy(&t);
                st = dz_limited_check_tlv_at(d, offset, type,
                                start + TLV_SIZEOF_HEADER, next, parents);

                if (st == 1) {
                        push_offset(parents, start);
                }
                return st;
        }
        if (ttype == TLV_DATED) {
                tlv_destroy(&t);
                st = dz_limited_check_tlv_at(d, offset, type,
                                start + TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE,
                                next, parents);
                if (st == 1) {
                        push_offset(parents, start);
                }
                return st;
        }

        tlv_destroy(&t);
        return 0;
}

int dz_check_tlv_at(dz_t *d, off_t offset, int type, off_t **parents) {
        int st;

        if (d == NULL) {
                return DZ_NULL_POINTER_ERROR;
        }
        if (offset < DAZIBAO_HEADER_SIZE) {
                return DZ_OFFSET_ERROR;
        }

        if (parents != NULL) {
                *parents = (off_t*)calloc(TLV_MAX_DEPTH, sizeof(off_t));
                if (*parents == NULL) {
                        return DZ_MEMORY_ERROR;
                }
        }

        st = dz_limited_check_tlv_at(d, offset, type, DAZIBAO_HEADER_SIZE,
                        d->len, parents == NULL ? NULL : *parents);

        if (parents != NULL && st != 1) {
                free(*parents);
                *parents = NULL;
        }

        return st;
}

int dz_do_empty(dz_t *d, off_t start, off_t length) {

        int len, status;
        tlv_t buff;
        char pad1s[TLV_SIZEOF_HEADER-1];
        char *b_val;

        len = MIN(length, TLV_MAX_VALUE_SIZE);
        b_val = calloc(len, sizeof(char));

        if (b_val == NULL) {
                status = -1;
                goto OUT;
        }

        tlv_init(&buff);
        tlv_set_type(&buff, TLV_PADN);
        tlv_set_length(&buff, len);
        tlv_mread(&buff, b_val);
        status = 0;

        if (d == NULL || start < DAZIBAO_HEADER_SIZE || length < 0) {
                status = -1;
                goto OUT;
        }

        if (length == 0) {
                goto OUT;
        }

        d->offset = start;

        while (length > (TLV_SIZEOF_HEADER + 2)) {
                int tmp = length;
                if (length > TLV_MAX_SIZE) {
                        length = TLV_MAX_SIZE;
                }

                /* set type */
                tlv_set_type(&buff, TLV_PADN);
                /* set length */
                tlv_set_length(&buff, length - TLV_SIZEOF_HEADER);

                /* XXX the TLV's length is 4 */
                if(dz_write_tlv_at(d, &buff, start) == -1) {
                        status = -1;
                        goto OUT;
                }

                start = start + length;
                length = tmp - length;
        }


        /* We don't have enough room to store a padN, so we fill it with
         * pad1's
         */
        if (length > 0) {
                memset(d->data + d->offset, TLV_PAD1, length);
        }

OUT:
        tlv_destroy(&buff);
        free(b_val);
        return status;
}


int dz_compact(dz_t *d) {
        tlv_t tlv;
        tlv_init(&tlv);
        off_t reading = DAZIBAO_HEADER_SIZE,
              writing = -1;

        int status = 0;

        char buff[BUFFLEN];

        if (d == NULL) {
                status = -1;
                goto OUT;
        }

        d->offset = reading;

        while ((reading = dz_next_tlv(d, &tlv)) != EOD) {
                int type = tlv_get_type(&tlv);

                if ((type == TLV_PAD1) || (type == TLV_PADN)) {
                        if (writing == -1) {
                                writing = reading;
                        }
                } else {
                        if (writing != -1) {
                                d->offset = (reading + TLV_SIZEOF_HEADER);

                                int len = tlv_get_length(&tlv);

                                if (tlv_mread(&tlv, d->data + d->offset)) {
                                        status = -1;
                                        goto OUT;
                                }

                                if (dz_write_tlv_at(d, &tlv, writing) == -1) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }

                                writing = d->offset;
                                d->offset = reading + TLV_SIZEOF(&tlv);
                        }
                }
        }

        if (writing != -1) {
            if (ftruncate(d->fd, writing) < 0) {
                    perror("ftruncate");
                    status = -1;
                    goto OUT;
            }
            d->len = writing;
        }

OUT:
        tlv_destroy(&tlv);
        return status;
}

int dz_dump(dz_t *daz_buf, off_t end, int depth, int indent,
                int flag_debug) {

        tlv_t tlv;
        off_t off;
        char *ind;

        if (tlv_init(&tlv) == -1) {
                return -1;
        }

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

                tlv_type = tlv_get_type(&tlv);
                len = tlv_get_length(&tlv);

                tlv_str = tlv_type2str((char) tlv_type);
                /* for option debug print pad n and pad1 only debug = 1 */
                if (!TLV_IS_EMPTY_PAD(tlv_type) || flag_debug) {
                        printf("%s%9li | %8s | %8d\n",
                                ind, (long)off, tlv_str, len);
                }

                switch (tlv_type) {
                case TLV_DATED:
                        if (depth > 0) {
                                off_t current = daz_buf->offset;
                                daz_buf->offset = off + TLV_SIZEOF_HEADER
                                        + TLV_SIZEOF_DATE;
                                if (dz_dump(daz_buf, current, (depth - 1),
                                                (indent + 1), flag_debug)) {
                                        free(ind);
                                        return -1;
                                }
                                daz_buf->offset = current;
                        }
                        break;
                case TLV_COMPOUND:
                        if (depth > 0) {
                                off_t current = daz_buf->offset;
                                daz_buf->offset = off + TLV_SIZEOF_HEADER;
                                if (dz_dump(daz_buf, current, (depth - 1),
                                                (indent + 1), flag_debug)) {
                                        free(ind);
                                        return -1;
                                }
                                daz_buf->offset = current;
                        }
                        break;
                }
        }

        free(ind);
        tlv_destroy(&tlv);
        return 0;
}

#undef BUFFLEN
