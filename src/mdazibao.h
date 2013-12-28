#ifndef _MDAZIBAO_H
#define _MDAZIBAO_H 1

#include <sys/types.h>
#include "utils.h"
#include "tlv.h"

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
#define EOD -2

/**
 * The magic number used to identify a Dazibao file
 **/
#define MAGIC_NUMBER 53

/**
 * The type of a Dazibao
 **/
typedef struct {
        /** file descriptor */
        int fd;
        /** flag used to open the file */
        int fflags;
        /** length of the dazibao */
        size_t len;
        /** current offset */
        size_t offset;
        /** actual length of the file */
        size_t space;
        /** mmaped region */
        char *data;
} dz_t;

/** type of a Dazibao hash */
typedef int hash_t;

/**
 * @param d
 * @param off
 **/
int dz_set_offset(dz_t *d, off_t off);

/**
 * @param d
 **/
off_t dz_get_offset(dz_t *d);

/**
 * @param d
 * @param off
 **/
int dz_incr_offset(dz_t *d, off_t off);

/**
 * @param d the dazibao
 **/
int dz_sync(dz_t *d);

/**
 * @param d the dazibao
 * @param t
 **/
int dz_mmap_data(dz_t *d, size_t t);

/**
 * @param d the dazibao
 * @param t
 **/
int dz_remap(dz_t *d, size_t t);


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
int dz_create(dz_t *daz_buf, char *path);

/**
 * Open a dazibao.
 * @param d dazibao to fill with information
 * @param path location where to find the file
 * @param flags flags used with open(2)
 * @return 0 on success, -1 on error
 * @see dz_open_with_size
 **/
int dz_open(dz_t *d, char *path, int flags);

/**
 * Same as dz_open, but let you choose the size of the mmapping (see #129).
 * @param d dazibao to fill with information
 * @param path location where to find the file
 * @param flags flags used with open(2)
 * @param size number of bytes to add to the file size for the mmapping.
 * @return 0 on success, -1 on error
 * @see dz_open
 **/
int dz_open_with_size(dz_t *d, char *path, int flags, size_t size);

/**
 * Close a dazibao
 * @param d the dazibao to close
 * @return 0 on success
 **/
int dz_close(dz_t *d);

/**
 * Reset the cursor of a dazibao for further readings.
 * @param d the dazibao to reset
 * @return 0 on success
 **/
int dz_reset(dz_t *d);

/**
 * Fill tlv value
 * @param d dazibao used for reading
 * @param tlv tlv to be filled
 * @param offset offset of the TLV
 * @return 0 on success, -1 on error
 **/
int dz_read_tlv(dz_t *d, tlv_t *tlv, off_t offset);

/**
 * Read a 4-bytes date in a dazibao
 * @param d the dazibao
 * @param offset offset of the date
 * @return a timestamp
 **/
time_t dz_read_date_at(dz_t *d, off_t offset);

/**
 * Check that a TLV has a good type by reading its first bytes. This is
 * especially useful with media types to verify that we're using the
 * appropriate file type. Note: some TLVs are not checked.
 * @param dz pointer to the dazibao
 * @param offset offset of the TLV
 * @return 1 if the type is ok, 0 if not
 **/
char dz_check_tlv_type(dz_t *dz, off_t offset);

/**
 * Extract info from a TLV image and store it a struct.
 * @param dz a pointer to the dazibao
 * @param offset the TLV's offset
 * @param info the struct for the TLV image infos
 * @return 0 on success
 **/
int dz_get_tlv_img_infos(dz_t *dz, off_t offset, struct img_info *info);

/**
 * Fill a tlv with the type and the length of the next TLV in the Dazibao
 * @param d dazibao used for reading
 * @param tlv to be filled
 * @return the offset on success, EOD if the end of file has been reached, or
 *         -1 on error
 **/
off_t dz_next_tlv(dz_t *d, tlv_t *tlv);

/**
 * Fill a tlv with its type and its length
 * @param d dazibao used for reading
 * @param tlv to be filled
 * @param offset position of the tlv wanted in d
 * @return 0 on success, -1 on error
 **/
int dz_tlv_at(dz_t *d, tlv_t *tlv, off_t offset);

/**
 * Write a tlv in a dazibao at a given offset
 * @param d the dazibao to write in
 * @param tlv the tlv to write
 * @param offset the offset to write at
 * @return 0 on success
 **/
int dz_write_tlv_at(dz_t *d, tlv_t *tlv, off_t offset);

/**
 * Add a TLV at the end of a dazibao. If the dazibao ends with a sequence of
 * pad1/padN's, the TLV overrides the beginning of the sequence, and the file
 * is truncated if the TLV is smaller than the total size of the sequence.
 * @param d dazibao to add the tlv to
 * @param tlv to add
 * @return 0 on success
 **/
int dz_add_tlv(dz_t *d, tlv_t *tlv);

/**
 * Erase a tlv. If tlv is surrounded by pad1/padNs, they will be concatened. If
 * it leaves pad1/padN at the end of dazibao, it will be truncated.
 * @param d dazibao from which remove this TLV
 * @param offset offset of the tlv to remove
 * @return 0 on success, -1 on error
 **/
int dz_rm_tlv(dz_t *d, off_t offset);

/**
 * Check that a Dazibao contains a TLV of a known type at a given offset. This
 * verifies that this TLV is either a top-level TLV or contained in a
 * compound/dated one. This doesn't check its type, use dz_check_tlv_type for
 * that
 * @param d a pointer to a Dazibao opened at least with the rights to read in
 * it
 * @param offset the offset of the TLV
 * @param type the type of the TLV. If this is -1, it'll won't be verified, and
 * any known TLV will work.
 * @param parents if not null, will be filled with an array of offsets for the
 * TLVs containing the checked TLV. If it's a top-level TLV, this array will
 * only contain its offset. If it's a child of a top-level TLV, it'll contain
 * the offset of the checked TLV followed by the offset of the top-level TLV,
 * etc. This array will contain at most TLV_MAX_DEPTH elements. If it contains
 * less than TLV_MAX_DEPTH offsets, the other ones are set to (off_t)0.
 * @return 1 if there's such TLV, 0 if there's not, a negative number on error
 * @see dz_check_tlv_type
 **/
int dz_check_tlv_at(dz_t *d, off_t offset, int type, off_t **parents);

/**
 * Empty a part of a dazibao.The part is filled with padN/pad1
 * @param d the Dazibao
 * @param start starting offset of emptying
 * @param length number of bytes to be erased
 * @return 0 on success, -1 on error
 **/
int dz_do_empty(dz_t *d, off_t start, off_t length);

/**
 * Compact a Dazibao file. The file must have been opened in read/write mode,
 * and the Dazibao is NOT closed by the function.
 * @return number of bytes saved by the compacting operation on success, or -1
 *         on error
 **/
int dz_compact(dz_t *d);

/**
 * @param d
 * @param depth
 * @param flag_debug
 **/
int dz_dump_all(dz_t *d, int depth, int flag_debug);

/**
 * dump information of tlv contained in a dazibao on standard output
 * with possible option depth and debug
 * @param daz_buf
 * @param end
 * @param depth
 * @param indent
 * @param flag_debug
 */
int dz_dump(dz_t *daz_buf, off_t end, int depth, int indent, int flag_debug);

/**
 * Create TLV with using dz for tlv type compound
 * @param dz
 * @param tlv
 * @return size of new tlv
 **/
int dz2tlv(char *dz, tlv_t *tlv);

/**
 * Test if a Dazibao changed using an hash. This function is really simple, the
 * notifications server should be used to more accurate results.
 * @param dz a pointer on the dazibao
 * @param oldhash a pointer to a int containing the previous hash. If it's set
 * to 0, we'll assume that there was no previous hash. The function will
 * compare it to the new hash then store the new one in it.
 * @return 1 if the new hash is different of the previous one, 0 if it's the
 * same or if *oldhash was 0. A negative number is returned on error.
 **/
int dz_hash(dz_t *dz, hash_t *oldhash);


/**
 * TODO document this function
 * CAREFUL: This function return a pointer to memory allocated with malloc and
 * should be freed after use offset should be pointing just after the LONGH
 * corresponding
 * @param dz dazibao used
 * @param tlv long tlv to extract
 * @param len length of the tlv value
 * @return pointer to buffer on success, NULL on error
 */
char *dz_get_ltlv_value(dz_t *dz, tlv_t *tlv, uint32_t len);

#endif /* _DAZIBAO_H */
