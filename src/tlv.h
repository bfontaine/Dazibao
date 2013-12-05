#ifndef _TLVS_H
#define _TLVS_H 1

/** @file
 * Set of functions used to work with TLVs
 **/

/** code for a Pad1 TLV */
#define TLV_PAD1     0

/** code for a PadN TLV */
#define TLV_PADN     1

/** code for a TEXT TLV */
#define TLV_TEXT     2

/** code for a PNG TLV */
#define TLV_PNG      3

/** code for a JPEG TLV */
#define TLV_JPEG     4

/** code for a compound TLV */
#define TLV_COMPOUND 5

/** code for a dated TLV */
#define TLV_DATED    6

/** size of the date field in a dated TLV */
#define TLV_SIZEOF_DATE 4

/** size of the type field in a TLV header */
#define TLV_SIZEOF_TYPE 1

/** size of the length field in a TLV header */
#define TLV_SIZEOF_LENGTH 3

/** size of a TLV header */
#define TLV_SIZEOF_HEADER (TLV_SIZEOF_TYPE + TLV_SIZEOF_LENGTH)

/** size of a whole TLV */
#define TLV_SIZEOF(t) (TLV_SIZEOF_TYPE+(tlv_get_type(t)==TLV_PAD1 \
                                ? 0                               \
                                : TLV_SIZEOF_LENGTH+tlv_get_length((t))))

/** test if a TLV is a Pad1 or a PadN */
#define TLV_IS_EMPTY_PAD(t) ((t) == TLV_PAD1 || (t) == TLV_PADN)

/** maximum size of a TLV value */
#define TLV_MAX_VALUE_SIZE ((1<<((TLV_SIZEOF_LENGTH)*8))-1)

/** maximum size of a whole TLV */
#define TLV_MAX_SIZE ((TLV_SIZEOF_HEADER)+(TLV_MAX_VALUE_SIZE))

/**
 * The type of a TLV
 **/
typedef char* tlv_t;

/**
 * Initialize a TLV. If the TLV was previously initialized/filled, call
 * tlv_destroy on it before calling this function.
 **/
int tlv_init(tlv_t *t);

/**
 * Destroy a TLV.
 **/
int tlv_destroy(tlv_t *t);

/**
 * @param tlv
 * @return type of tlv
 **/
int tlv_get_type(tlv_t tlv);

/**
 * Set type of a tlv.
 * @param tlv tlv whose type has to be set
 * @param t type to set
 **/
void tlv_set_type(tlv_t *tlv, char t);

/**
 * Set length of a tlv.
 * @param tlv tlv whose length has to be set
 * @param n length to set
 **/
void tlv_set_length(tlv_t *tlv, unsigned int n);

/**
 * Get the adress of a tlv length.
 * @param tlv
 * @return a pointer to the length field of tlv
 **/
tlv_t tlv_get_length_ptr(tlv_t tlv);

/**
 * Get length of a tlv.
 * @param tlv tlv whose length is asked
 * @return value of tlv's length
 **/
unsigned int tlv_get_length(tlv_t tlv);

/**
 * Get the adress of a tlv value.
 * @param tlv
 * @return a pointer to the value field of tlv
 **/
tlv_t tlv_get_value_ptr(tlv_t tlv);

/**
 * Write a tlv.
 * Offset of {fd} when returning is unspecified.
 * @param tlv tlv to write
 * @param fd file descriptor where tlv is written
 * @return 0 on success
 * @return -1 on error
 **/
int tlv_write(tlv_t tlv, int fd);

/**
 * Read a tlv value.
 * Offset of {fd} when returning is set after the value read on success, and
 * unspecified on error.
 * @param tlv tlv to write
 * @param fd file descriptor from which value is read
 * @return 0 on success
 * @return -1 on error
 **/
int tlv_read(tlv_t *tlv, int fd);

/**
 * Write a whole tlv in a file
 * @param tlv tlv to write
 * @param fd file descriptor where you want to dump
 * @return same as write(2)
 **/
int dump_tlv(tlv_t tlv, int fd);


/**
 * Write the value of a tlv in a file
 * @param tlv tlv containing the value
 * @param fd file descriptor where you want to dump
 * @return same as write(2)
 **/
int dump_tlv_value(tlv_t tlv, int fd);

/**
 * Return a string representation for a TLV type
 **/
const char *tlv_type2str(char tlv_type);

#endif
