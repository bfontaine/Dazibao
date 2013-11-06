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

typedef int daz_t;

/*
 * Create a new dazibao in a file given by 'path', and fill 'daz_buf'
 * accordingly. If a file already exist, the function doesn't override it
 * and return an error status instead.
 * Returns 0 on success and -1 on error.
 */
int create_dazibao(daz_t *daz_buf,  char *path);

/*
 * Open a dazibao. 'flags' flags are passed to the open(2) call used to open
 * the file. Returns 0 on success, -1 on error.
 */
int open_dazibao(daz_t* d,  char* path,  int flags);

/*
 * Close a dazibao. Returns 0 on success.
 */
int close_dazibao(daz_t* d);

/*
 * Fill all the fields of 'buf' with the TLV located at 'offset' offset in the
 * dazibao 'd'. The function returns 0 on success and -1 on error.
 */
int read_tlv(daz_t* d, char *tlv,  off_t offset);

/*
 * Fill the type and length attributes of 'buf' with the TLV located at the
 * current offset in the dazibao 'd'. The function returns the offset of this
 * TLV on success, EOD if the end of file has been reached, and -1 on error.
 */
off_t next_tlv(daz_t* d, char *tlv);

/*
 * Fill the type and length attributes of 'buf' with the TLV located at offset
 * 'offset' in the dazibao. The current offset is not modified.
 * The function returns 0 on success and -1 on error.
 */
int tlv_at(daz_t* d, char *tlv,  off_t offset);

/*
 * Add a TLV at the end of a dazibao. If the dazibao ends with a sequence
 * of pad1/padN's, the TLV 'src' overrides the beginning of the sequence,
 * and the file is truncated if the TLV is smaller than the total size of the
 * sequence.
 */
int add_tlv(daz_t* d, char *tlv);

/*
 * Return the offset of the first pad1/padN of a possibly empty sequence of
 * pad1/padN's before 'offset'. If the sequence is empty, the given offset
 * is returned.
 */
off_t pad_serie_start (daz_t* d,  off_t offset);

/*
 * Return the offset of the next TLV after 'offset' which is not a pad1
 * nor padN. If there is none, the end of file offset is returned.
 * See also 'pad_serie_start'.
 */
off_t pad_serie_end(daz_t* d,  off_t offset);

/*
 * Remove the TLV at 'offset' from a dazibao.
 */
int rm_tlv(daz_t* d,  off_t offset);

/*
 * Empty a part of a dazibao, starting at 'start', and of length 'length'.  The
 * part is filled with padN's and pad1's.
 */
int empty_dazibao(daz_t *d, off_t start, off_t length);

/*
 * Compact a Dazibao file. The file must have been opened in read/write mode,
 * and the Dazibao is NOT closed by the function. Also, the dazibao offset is
 * NOT preserved.
 * The function returns the number of bytes saved by the compacting operation,
 * or -1 if an error occured.
 */
int compact_dazibao(daz_t *d);


/*
 * print tlvs contained in 'daz_buf' on standard output
 */
int dump_dazibao(daz_t *daz_buf);

#endif /* _DAZIBAO_H */
