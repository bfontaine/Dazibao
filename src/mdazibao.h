#ifndef _MDAZIBAO_H
#define _MDAZIBAO_H 1

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include "tlv.h"
#include "utils.h"

/**
 * @file
 * Dazibao API
 **/

/**
 * Size of a Dazibao header
 **/
#define DAZIBAO_HEADER_SIZE 4

/**
 * code used to represent "End Of Dazibao", similarly to EOF.
 **/
#define EOD 0

/**
 * The magic number used to identify a Dazibao file
 **/
#define MAGIC_NUMBER 53

/**
 * The type of a Dazibao
 **/
typedef struct {
        int fd;
        size_t len;
        size_t offset;
        char *data;
} mdz_t;

/**
 * Create a new dazibao in a file at a given location.
 * If a file already exist, the function doesn't override it
 * and return an error status instead.
 * @param daz_buf a pointer on a dazibao which will be filled with the
 *                data associated with this Dazibao
 * @param path path to the new file. The file must not already exist and all
 *        the directories before it must exist.
 * @return 0 on success, -1 on error
 **/
int mdz_create(mdz_t *daz_buf, char *path);

/**
 * Open a dazibao.
 * @param d dazibao to fill with information
 * @param path location where to find the file
 * @param flags flags used with open(2)
 * @return 0 on success, -1 on error
 **/
int mdz_open(mdz_t *d, char *path, int flags);

/**
 * Close a dazibao
 * @param d the dazibao to close
 * @return 0 on success
 **/
int mdz_close(mdz_t *d);

/**
 * Reset the cursor of a dazibao for further readings.
 * @param d the dazibao to reset
 * @return 0 on success
 **/
int mdz_reset(mdz_t *d);

/**
 * Fill tlv value
 * @param d dazibao used for reading
 * @param tlv tlv to be filled
 * @param offset off wanted tlv
 * @return 0 on success, -1 on error
 **/
int mdz_read_tlv(mdz_t *d, tlv_t *tlv, off_t offset);

/**
 * Fill a tlv with the type and the length of the next TLV in the Dazibao
 * @param d dazibao used for reading
 * @param tlv to be filled
 * @return the offset on success, EOD if the end of file has been reached, or
 *         -1 on error
 **/
off_t mdz_next_tlv(mdz_t *d, tlv_t *tlv);

/**
 * Fill a tlv with its type and its length
 * @param d dazibao used for reading
 * @param tlv to be filled
 * @param offset position of the tlv wanted in d
 * @return 0 on success, -1 on error
 **/
int mdz_tlv_at(mdz_t *d, tlv_t *tlv, off_t offset);

/**
 * Write a tlv in a dazibao at a given offset
 * @param d the dazibao to write in
 * @param tlv the tlv to write
 * @param offset the offset to write at
 * @return 0 on success
 **/
int mdz_write_tlv_at(mdz_t *d, tlv_t *tlv, off_t offset);

/**
 * Add a TLV at the end of a dazibao. If the dazibao ends with a sequence of
 * pad1/padN's, the TLV overrides the beginning of the sequence, and the file
 * is truncated if the TLV is smaller than the total size of the sequence.
 * @param d dazibao to add the tlv to
 * @param tlv to add
 * @return 0 on success
 **/
int mdz_add_tlv(mdz_t *d, tlv_t *tlv);

/**
 * Erase a tlv. If tlv is surrounded by pad1/padNs, they will be concatened. If
 * it leaves pad1/padN at the end of dazibao, it will be truncated.
 * @param d dazibao from which remove this TLV
 * @param offset offset of the tlv to remove
 * @return 0 on success, -1 on error
 **/
int mdz_rm_tlv(mdz_t *d, off_t offset);

/**
 * Empty a part of a dazibao.The part is filled with padN/pad1
 * @param d the Dazibao
 * @param start starting offset of emptying
 * @param length number of bytes to be erased
 * @return 0 on success, -1 on error
 **/
int mdz_do_empty(mdz_t *d, off_t start, off_t length);

/**
 * Compact a Dazibao file. The file must have been opened in read/write mode,
 * and the Dazibao is NOT closed by the function. Also, the dazibao offset is
 * NOT preserved.
 * @return number of bytes saved by the compacting operation on success, or -1
 *         on error
 **/
int mdz_compact(mdz_t *d);

/**
 * dump information of tlv contained in a dazibao on standard output
 * with possible option depth and debug
 * @param daz_buf
 */
int mdz_dump(mdz_t *daz_buf, off_t end, int depth, int indent, int flag_debug);

#endif /* _DAZIBAO_H */
