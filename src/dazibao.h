#ifndef _DAZIBAO_H
#define _DAZIBAO_H 1

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "tlv.h"
#include "utils.h"

#define DAZIBAO_HEADER_SIZE 4
#define EOD 0
#define MAGIC_NUMBER 53

typedef int dz_t;

/**
 * Create a new dazibao in a file at a given location.
 * If a file already exist, the function doesn't override it
 * and return an error status instead.
 * @param daz_buf dazibao to fill with information
 * @param path location where to create the file
 * @return 0 on success
 * @return -1 on error
 */
int dz_create(dz_t *daz_buf, char *path);

/**
 * Open a dazibao.
 * @param dad_buf dazibao to fill with information
 * @param path location where to find the file
 * @param flags flags used with open(2)
 * @return 0 on succes
 * @return -1 on error
 */
int dz_open(dz_t *d, char *path, int flags);

/**
 * Close a dazibao
 * @return 0 on success
 */
int dz_close(dz_t *d);

/**
 * Fill tlv value
 * @param d dazibao used for reading
 * @param tlv tlv to be filled
 * @param offset off wanted tlv
 * @return 0 on success
 * @return -1 on error
 */
int dz_read_tlv(dz_t *d, tlv_t *tlv, off_t offset);

/**
 * Fill tlv with type and length information
 * @param d dazibao used for reading
 * @param tlv to be filled
 * @return offset this tlv on success
 * @return EOD if end of file reached
 * @return -1 on error
 */
off_t dz_next_tlv(dz_t *d, tlv_t *tlv);

/**
 * Fill tlv with type and length information
 * @param d dazibao used for reading
 * @param tlv to be filled
 * @param offset position of the tlv wanted in d
 * @return 0 on success
 * @return -1 on error
 */
int dz_tlv_at(dz_t *d, tlv_t *tlv, off_t offset);

/**
 * Write a tlv in a dazibao at a given offset
 **/
int dz_write_tlv_at(dz_t *d, tlv_t tlv, off_t offset);

/**
 * Add a TLV at the end of a dazibao. If the dazibao ends with a sequence
 * of pad1/padN's, the TLV overrides the beginning of the sequence,
 * and the file is truncated if the TLV is smaller than the total size of the
 * sequence.
 * @param d dazibao receiving new tlv
 * @param tlv to add
 */
int dz_add_tlv(dz_t *d, tlv_t tlv);

/**
 * Look for the beggining of an unbroken pad1/padN serie leading to `offset`.
 * @return offset of the begging of this serie on search succes
 * @return {offset} if search was unsuccessful
 */
off_t dz_pad_serie_start(dz_t *d, off_t offset);

/**
 * Skip tlv at offset, and look for the end of an unbroken pad1/padN serie
 * starting after the skipped tlv.
 * @return offset of the end of this serie on search succes
 * @return offset of next tlv after {offset} if search was unsuccessful
 */
off_t dz_pad_serie_end(dz_t *d, off_t offset);

/**
 * Erase a tlv. If tlv is surrounded by pad1/padN, they will be concatened.
 * If it leaves pad1/padN at the end of dazibao, it will be truncated
 * @param d dazibao where is tlv to remove
 * @param offset offset of the tlv to remove
 */
int dz_rm_tlv(dz_t *d, off_t offset);

/**
 * Empty a part of a dazibao.The part is filled with padN/pad1
 * @param start starting offset of emptying
 * @param length number of bytes to be erased
 */
int dz_do_empty(dz_t *d, off_t start, off_t length);

/**
 * Compact a Dazibao file. The file must have been opened in read/write mode,
 * and the Dazibao is NOT closed by the function.
 * Also, the dazibao offset is NOT preserved.
 * @return number of bytes saved by the compacting operation on success
 * @return -1 on error
 */
int dz_compact(dz_t *d);


/**
 * dump information of tlv contained in a dazibao on standard output
 * @param daz_buf
 */
int dz_dump(dz_t *daz_buf);

/*
 * print tlvs contained in 'daz_buf' with option depth
 * on standard output
 */
int dz_dump_compound(dz_t *daz_buf, off_t end, int depth, int indent);

#endif /* _DAZIBAO_H */
