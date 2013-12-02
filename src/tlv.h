#ifndef _TLVS_H
#define _TLVS_H 1

#define TLV_PAD1     0
#define TLV_PADN     1
#define TLV_TEXT     2
#define TLV_PNG      3
#define TLV_JPEG     4
#define TLV_COMPOUND 5
#define TLV_DATED    6

#define TLV_TEXT_STR     "TEXT"
#define TLV_PNG_STR      "PNG"
#define TLV_JPEG_STR     "JPEG"
#define TLV_COMPOUND_STR "COMPOUND"
#define TLV_DATED_STR    "DATE"

#define TLV_SIZEOF_DATE 4
#define TLV_SIZEOF_TYPE 1
#define TLV_SIZEOF_LENGTH 3
#define TLV_SIZEOF_HEADER (TLV_SIZEOF_TYPE + TLV_SIZEOF_LENGTH)
#define TLV_SIZEOF(t) (TLV_SIZEOF_TYPE+(tlv_get_type(t)==TLV_PAD1 \
                                ? 0                               \
                                : TLV_SIZEOF_LENGTH+tlv_get_length((t))))

#define TLV_IS_EMPTY_PAD(t) ((t) == TLV_PAD1 || (t) == TLV_PADN)

#define TLV_MAX_VALUE_SIZE ((1<<((TLV_SIZEOF_LENGTH)*8))-1)
#define TLV_MAX_SIZE ((TLV_SIZEOF_HEADER)+(TLV_MAX_VALUE_SIZE))

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
 * Convert {n} in dazibao's endianess
 * and set {tlv}'s length field with the converted value.
 * @param n length wanted
 * @param tlv tlv receiving length
 * @deprecated use tlv_set_length instead
 */
void htod(unsigned int n, tlv_t tlv);

/**
 * Convert an int written in dazibao's endianess to host endianess.
 * @param len int using dazibao's endianess
 * @return value of length
 * @deprecated use get_length
 */
unsigned int dtoh(char *len);

/**
 * @param tlv
 * @return type of tlv
 */
int tlv_get_type(tlv_t tlv);

/**
 * Set type of a tlv.
 * @param tlv tlv whose type has to be set
 * @param t type to set
 */
void tlv_set_type(tlv_t *tlv, char t);

/**
 * Set length of a tlv.
 * @param tlv tlv whose length has to be set
 * @param n length to set
 */
void tlv_set_length(tlv_t *tlv, unsigned int n);

/**
 * Get the adress of a tlv length.
 * @param tlv
 * @return a pointer to the length field of tlv
 */
tlv_t tlv_get_length_ptr(tlv_t tlv);

/**
 * Get length of a tlv.
 * @param tlv tlv whose length is asked
 * @return value of tlv's length
 */
unsigned int tlv_get_length(tlv_t tlv);

/**
 * Get the adress of a tlv value.
 * @param tlv
 * @return a pointer to the value field of tlv
 */
tlv_t tlv_get_value_ptr(tlv_t tlv);

/**
 * Write a tlv.
 * Offset of {fd} when returning is unspecified.
 * @param tlv tlv to write
 * @param fd file descriptor where tlv is written
 * @return 0 on success
 * @return -1 on error
 */
int tlv_write(tlv_t tlv, int fd);

/**
 * Read a tlv value.
 * Offset of {fd} when returning is set after the value read on success, and
 * unspecified on error.
 * @param tlv tlv to write
 * @param fd file descriptor from which value is read
 * @return 0 on success
 * @return -1 on error
 */
int tlv_read(tlv_t *tlv, int fd);

/**
 * Write a whole tlv in a file
 * @param tlv tlv to write
 * @param fd file descriptor where you want to dump
 * @return same as write(2)
 */
int dump_tlv(tlv_t tlv, int fd);


/**
 * Write the value of a tlv in a file
 * @param tlv tlv containing the value
 * @param fd file descriptor where you want to dump
 * @return same as write(2)
 */
int dump_tlv_value(tlv_t tlv, int fd);

/**
 * Return a string representation for a TLV type
 **/
const char *tlv_type2str(char tlv_type);

#endif
