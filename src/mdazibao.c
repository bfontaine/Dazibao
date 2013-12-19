#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include "mdazibao.h"
#include "utils.h"
#include "tlv.h"

/** @file */

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

int dz_set_offset(dz_t *d, off_t off) {
        if (off > d->len) {
                return -1;
        }
        d->offset = off;
        return 0;
}

off_t dz_get_offset(dz_t *d) {
        return d->offset;
}

int dz_update_offset(dz_t *d, off_t off) {
        if (off > d->len) {
                return -1;
        }
        d->offset += off;
        return 0;
}

int dz_sync(dz_t *d) {
        if (ftruncate(d->fd, d->len) == -1) {
                PERROR("ftruncate");
                return -1;
        }

        return msync(d->data, d->len, MS_SYNC);
}

int dz_mmap_data(dz_t *d, size_t t) {

        size_t page_size, real_size;
        int prot;

        page_size = (size_t)sysconf(_SC_PAGESIZE);
        real_size = (size_t)(page_size * (t/page_size + MIN(1, t%page_size)));

        if (d->fflags == O_RDWR && ftruncate(d->fd, real_size) == -1) {
                PERROR("ftruncate");
                return -1;
        }

        prot = PROT_READ | (d->fflags == O_RDWR ? PROT_WRITE : 0);

        d->data = (char*)mmap(NULL, real_size, prot, MAP_SHARED, d->fd, 0);

        if (d->data == MAP_FAILED) {
                PERROR("mmap");
                return -1;
        }

        d->space = real_size;
        return 0;
}

int dz_remap(dz_t *d, size_t t) {

        if (d->fflags == O_RDONLY) {
                return DZ_RIGHTS_ERROR;
        }

        if (dz_sync(d) < 0
                        || munmap(d->data, d->len) == -1
                        || dz_mmap_data(d, t) == -1) {
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
        memset(d->data + 1, 0, DAZIBAO_HEADER_SIZE - 1);
        d->len = DAZIBAO_HEADER_SIZE;

        if (dz_sync(d) < 0) {
                return -1;
        }
        return 0;

PANIC:
        if (close(d->fd) == -1) {
                PERROR("close");
        }
        return -1;
}

int dz_open(dz_t *d, char *path, int flags) {
        return dz_open_with_size(d, path, flags, 0);
}

int dz_open_with_size(dz_t *d, char *path, int flags, size_t size) {

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

        if (dz_mmap_data(d, d->len + size) == -1) {
                goto PANIC;
        }

        if (d->data[0] != MAGIC_NUMBER || d->data[1] != 0) {
                goto PANIC;
        }

        return 0;
PANIC:
        if (close(d->fd) == -1) {
                PERROR("close");
        }
        return -1;
}

int dz_close(dz_t *d) {

        if (d->fflags == O_RDWR && dz_sync(d) < 0) {
                return DZ_MMAP_ERROR;
        }

        if (flock(d->fd, LOCK_UN) == -1) {
                perror("flock");
                return -1;
        }

        if (close(d->fd) == -1) {
                perror("close");
                return -1;
        }

        return 0;

}

int dz_read_tlv(dz_t *d, tlv_t *tlv, off_t offset) {
        return tlv_mread(tlv, d->data + offset + TLV_SIZEOF_HEADER);
}

time_t dz_read_date_at(dz_t *d, off_t offset) {
        uint32_t t = 0;
        memcpy(&t, d->data + offset, TLV_SIZEOF_DATE);
        return (time_t)ntohl(t);
}

char dz_check_tlv_type(dz_t *dz, off_t offset) {
        tlv_t t;
        char ok = 1;
        unsigned int length;

        tlv_init(&t);
        if (dz_tlv_at(dz, &t, offset) != 0) {
                tlv_destroy(&t);
                return -1;
        }

        length = tlv_get_length(&t);

        switch (tlv_get_type(&t)) {
        case TLV_PADN:
                if (length < TLV_MIN_PADN_LENGTH) {
                        ok = 0;
                }
                break;
        case TLV_DATED:
                if (length < TLV_SIZEOF_DATE) {
                        ok = 0;
                }
                break;
        default:
                ok = guess_type(dz->data + offset + TLV_SIZEOF_HEADER,
                                length) == tlv_get_type(&t);
        }

        tlv_destroy(&t);
        return ok;
}

int dz_get_tlv_img_infos(dz_t *dz, off_t offset, struct img_info *info) {
        tlv_t t;
        unsigned int length;
        off_t val_off;
        char st = -1;

        if (dz == NULL || info == NULL || offset < DAZIBAO_HEADER_SIZE) {
                return -1;
        }

        if (!dz_check_tlv_type(dz, offset)) {
                return -1;
        }

        tlv_init(&t);
        if (dz_tlv_at(dz, &t, offset) != 0) {
                tlv_destroy(&t);
                return -1;
        }

        length = tlv_get_length(&t);
        val_off = offset + TLV_SIZEOF_HEADER;

        switch (tlv_get_type(&t)) {
        case TLV_BMP:
                /* see second table at
                 *      en.wikipedia.org/wiki/BMP_file_format
                 *              #DIB_header_.28bitmap_information_header.29 */
                if (length < 0x20) {
                        st = -1;
                        break;
                }
                info->width = 0;
                info->height = 0;
                memcpy(&(info->width), dz->data + val_off + 0x12, 4);
                memcpy(&(info->height), dz->data + val_off + 0x16, 4);
                st = 0;
                break;
        case TLV_GIF:
                /* see www.onicos.com/staff/iz/formats/gif.html#header */
                if (length < 10) {
                        st = -1;
                        break;
                }
                info->width = ((unsigned char)dz->data[val_off + 7] << 8)
                        + (unsigned char)dz->data[val_off + 6];
                info->height = ((unsigned char)dz->data[val_off + 9] << 8)
                        + (unsigned char)dz->data[val_off + 8];
                st = 0;
                break;
        case TLV_JPEG:
                /* TODO
                 * see stackoverflow.com/a/692013/735926 */
                st = -1;
                break;
        case TLV_PNG:
                /* see stackoverflow.com/a/5354657/735926 */
                if (length < 24) {
                        st = -1;
                        break;
                }
                info->width = 0;
                info->height = 0;
                memcpy(&(info->width), dz->data + val_off + 16, 4);
                memcpy(&(info->height), dz->data + val_off + 20, 4);

                info->width = ntohl(info->width);
                info->height = ntohl(info->height);
                st = 0;
                break;
        case TLV_TIFF:
                /* TODO */
                st = -1;
                break;
        }

        tlv_destroy(&t);
        return st;
}

off_t dz_next_tlv(dz_t *d, tlv_t *tlv) {
        int type;
        int off_init = d->offset;

        if (d->len == d->offset) {
                return EOD;
        }
        if (d->len < d->offset + TLV_SIZEOF_TYPE) {
                return -1;
        }

        type = d->data[d->offset];

        tlv_set_type(tlv, type);
        d->offset += TLV_SIZEOF_TYPE;

        if (type == TLV_PAD1) {
                return off_init;
        }

        if (d->len < d->offset + TLV_SIZEOF_LENGTH) {
                return -1;
        }

        memcpy(tlv_get_length_ptr(tlv), d->data + d->offset,
                        TLV_SIZEOF_LENGTH);

        d->offset += TLV_SIZEOF_LENGTH + tlv_get_length(tlv);

        return off_init;
}

int dz_tlv_at(dz_t *d, tlv_t *tlv, off_t offset) {
        dz_t tmp = {
                -1,
                d->fflags,
                d->len,
                offset,
                d->space,
                d->data
        };
        return dz_next_tlv(&tmp, tlv);
}

int dz_write_tlv_at(dz_t *d, tlv_t *tlv, off_t offset) {
        return tlv_mwrite(tlv, d->data + offset);
}

int dz_add_tlv(dz_t *d, tlv_t *tlv) {

        off_t pad_off, eof_off;
        unsigned int tlv_size = TLV_SIZEOF(tlv);
        unsigned int available;

        eof_off = d->len;

        /* find offset of pad serie leading to EOF */
        pad_off = dz_pad_serie_start(d, eof_off, 0);
        available = d->space - pad_off;

        if (available < tlv_size) {
                if (dz_remap(d, d->space + (tlv_size - available)) == -1) {
                        return -1;
                }
        }

        d->len = pad_off + TLV_SIZEOF(tlv);

        /* write */
        if (dz_write_tlv_at(d, tlv, pad_off) < 0) {
                ERROR(NULL, -1);
        }

        if (dz_sync(d) < 0) {
                return -1;
        }
        return 0;
}

static off_t dz_pad_serie_start(dz_t *d, off_t offset, off_t min_offset) {

        off_t off_start, off_tmp, off_prev;
        tlv_t buf;

        tlv_init(&buf);

        off_prev = d->offset;
        d->offset = min_offset;
        off_start = -1;

        while ((off_tmp = dz_next_tlv(d, &buf)) != -1
                && off_tmp != EOD
                && off_tmp < offset) {
                if (TLV_IS_EMPTY_PAD(tlv_get_type(&buf)) && off_start == -1) {
                        /* we got a padN/pad1 */
                        off_start = off_tmp;
                } else {
                        /* not a padN/pad1, the serie is broken, reset
                           off_start */
                        off_start = -1;
                }
        }

        if (off_tmp == -1 || off_start == -1) {
                off_start = offset;
        }

        tlv_destroy(&buf);
        d->offset = off_prev;
        return off_start;
}

static off_t dz_pad_serie_end(dz_t *d, off_t offset, off_t max_offset) {
        off_t off_stop, off_prev;
        tlv_t buf;

        if (max_offset > 0 && max_offset < offset) {
                return DZ_OFFSET_ERROR;
        }

        tlv_init(&buf);
        off_prev = d->offset;
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
        d->offset = off_prev;
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
        off_t prev = d->offset;

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

                if (dz_write_tlv_at(d, &buff, start) == -1) {
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
        d->offset = prev;
        return status;
}

/**
 * Helper for 'inner_compact'. This is a wrapper around memmove to copy some
 * data between two offsets in a dazibao. If the offsets are the same, the
 * function avoid a call to memmove. It increments the offsets of the number of
 * copied bytes.
 * @param d the dazibao
 * @param reader pointer on the offset we need to copy from
 * @param writer pointer on the offset we need to copy to
 * @param count number of bytes to copy
 * @return 0
 **/
static int dz_memmove(dz_t *d, off_t *reader, off_t *writer, int count) {
        if (*reader == *writer) {
                /* nothing to copy here */
                *reader += count;
                *writer += count;
                return 0;
        }
        memmove(d->data + *writer, d->data + *reader, count);
        *writer += count;
        *reader += count;
        return 0;
}

/**
 * Helper for dz_compact. Compact a TLV at offset '*reader' and write it at
 * offset '*writer' in the current TLV.
 * @param d the dazibao
 * @param reader the offset we need to write at
 * @param writer the offset we need to read at
 * @param max_off the maximum offset we need to read to
 * @return the saved bytes count
 **/
static int compact_helper(dz_t *d, off_t *reader, off_t *writer,
                off_t max_off) {

        int type = -1,
            len = 0,
            saved = 0,
            is_dz = 0,
            bytescount;
        tlv_t t;
        off_t tlv_off,   /* offset where the TLV is written */
              value_off; /* offset of the beginning of the value */

        if (*reader > max_off || *reader < 0 || *writer < 0) {
                return 0;
        }

        if (*reader == max_off) {
                return 0;
        }

        /* We're in a Dazibao, which is almost the same as in a
           compound except that we don't have to update the length. */
        is_dz = (*reader == 0);

        if (!is_dz) {
                tlv_init(&t);
                dz_tlv_at(d, &t, *reader);

                type = tlv_get_type(&t);
                len  = tlv_get_length(&t);
        } else {
                *reader += DAZIBAO_HEADER_SIZE;
                *writer += DAZIBAO_HEADER_SIZE;
        }

        switch (type) {
        case TLV_PAD1:
                saved = TLV_SIZEOF_TYPE;
                *reader += saved;
                goto EOCOMPACT;
        case TLV_PADN:
                saved = TLV_SIZEOF_HEADER + len;
                *reader += saved;
                goto EOCOMPACT;
        case TLV_DATED:
                tlv_off = *writer;
                /* copy the type */
                dz_memmove(d, reader, writer, TLV_SIZEOF_TYPE);
                /* don't copy the length now, we'll set it later because it
                   may change */
                *writer += TLV_SIZEOF_LENGTH;
                *reader += TLV_SIZEOF_LENGTH;
                /* value (date + inner TLV) */
                value_off = *writer;
                dz_memmove(d, reader, writer, TLV_SIZEOF_DATE);
                len -= TLV_SIZEOF_DATE;
                saved = compact_helper(d, reader, writer, *reader + len);
                len = *writer - value_off; /* new length */
                if (len == TLV_SIZEOF_DATE) {
                        /* empty TLV, we're removing it (#97) */
                        *writer = tlv_off;
                        saved += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                }

                tlv_set_length(&t, len);
                memmove(d->data + tlv_off + TLV_SIZEOF_TYPE,
                                t + TLV_SIZEOF_TYPE, TLV_SIZEOF_LENGTH);
                goto EOCOMPACT;
        case -1:
        case TLV_COMPOUND:
                tlv_off = *writer;
                if (!is_dz) {
                        /* copy the type */
                        dz_memmove(d, reader, writer, TLV_SIZEOF_TYPE);
                        /* don't copy the length of now, we'll set it later
                         * because it may change */
                        *writer += TLV_SIZEOF_LENGTH;
                        *reader += TLV_SIZEOF_LENGTH;
                        max_off = tlv_off + TLV_SIZEOF_HEADER + len;
                }
                while (*reader < max_off) {
                        saved += compact_helper(d, reader, writer, max_off);
                }
                if (is_dz) {
                        d->len = *writer;
                } else {
                        len = *writer - (tlv_off + TLV_SIZEOF_HEADER);
                        if (len == 0) {
                                /* empty TLV, we're removing it (#97) */
                                *writer = tlv_off;
                                saved += TLV_SIZEOF_HEADER;
                        }
                        tlv_set_length(&t, len);
                        memmove(d->data + tlv_off + TLV_SIZEOF_TYPE,
                                t + TLV_SIZEOF_TYPE, TLV_SIZEOF_LENGTH);
                }
                goto EOCOMPACT;
        default: /* other TLVs */
                if (len == 0) {
                        saved += TLV_SIZEOF_HEADER;
                        *reader += TLV_SIZEOF_HEADER;
                } else {
                        dz_memmove(d, reader, writer, TLV_SIZEOF_HEADER + len);
                }
        }

EOCOMPACT:
        if (!is_dz) {
                tlv_destroy(&t);
        }
        return saved;
}

int dz_compact(dz_t *d) {
        off_t reader = 0,
              writer = 0;

        return compact_helper(d, &reader, &writer, d->len);
}

int dz_dump_all(dz_t *d, int depth, int flag_debug) {
        return dz_dump(d, d->len, depth, 0, flag_debug);
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

                tlv_str = tlv_type2str(tlv_type);
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
                                /* TODO add a function to print a date */
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

int dz2tlv(char *d, tlv_t *tlv) {
        dz_t dz;

        if (dz_open(&dz, d, O_RDWR) < 0) {
                fprintf(stderr, "Error while opening the dazibao\n");
                return -1;
        }

        tlv_set_type(tlv, (char) TLV_COMPOUND);
        tlv_set_length(tlv, dz.len - DAZIBAO_HEADER_SIZE );
        tlv_mread(tlv, dz.data + DAZIBAO_HEADER_SIZE );

        if (dz_close(&dz) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                return -1;
        }

        return TLV_SIZEOF(tlv);
}

int dz_hash(dz_t *dz, hash_t *oldhash) {
        hash_t len;
        if (dz == NULL || oldhash == NULL) {
                return DZ_NULL_POINTER_ERROR;
        }

        /* TODO this is a basic function, this code should be replaced by a
         * real hashing function */

        len = (hash_t)dz->len;

        if (*oldhash == 0 || *oldhash == len) {
                *oldhash = len;
                return 0;
        }

        *oldhash = len;
        return 1;
}
#undef BUFFLEN
