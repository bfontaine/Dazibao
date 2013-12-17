#ifndef _TLVS_H
#define _TLVS_H 1

#include <stdio.h>
#include <stdint.h>

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

/** code for a GIF TLV (see #100) */
#define TLV_GIF    140

#define TLV_TIFF   128

#define TLV_MIN_PADN_LENGTH 2

/** test that a TLV type is valid */
#define TLV_VALID_TYPE(type) (0 < (type) && (type) <= 255)

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

/** arbitrary TLV depth limit. This is NOT a hard limit, but some functions
 * need to have a limit */
#define TLV_MAX_DEPTH 16

/**
 * The type of a TLV
 **/
typedef char* tlv_t;

/**
 * A TLV type
 **/
struct tlv_type {
        /* the type's code */
        int code;
        /* the type's name */
        char *name;
};

/**
 * A TLV type and its file signature
 **/
struct type_signature {
        /* TLV type */
        char type;
        /* File signature */
        char *signature;
};

/**
 * Guess type from a buffer
 * @param src data to test
 * @param len length of buffer
 * @return TLV type corresponding to the buffer
 */
unsigned char guess_type(char *src, unsigned int len);

/**
 * Convert an int written in host endianess into dazibao's one.
 * @param n length
 * @param len result parameter
 **/
void htod(unsigned int n, char *len);

/**
 * Convert an int written in dazibao's endianess to host endianess.
 * @param len int using dazibao's endianess
 * @return value of length
 **/
unsigned int dtoh(char *len);

/**
 * Initialize a TLV. If the TLV was previously initialized/filled, call
 * tlv_destroy on it before calling this function.
 * @param t the TLV to init
 * @return 0 on success
 **/
int tlv_init(tlv_t *t);

/**
 * Destroy a TLV.
 * @param t the TLV to destroy
 * @return 0 on success
 **/
int tlv_destroy(tlv_t *t);

/**
 * Return the type of a TLV
 * @param tlv
 * @return type of tlv
 **/
int tlv_get_type(tlv_t *tlv);

/**
 * Set the date of a (dated) TLV
 * @param tlv TLV whose date has to be set
 * @param date date to set
 **/
void tlv_set_date(tlv_t *tlv, uint32_t date);

/**
 * Set the type of a TLV
 * @param tlv TLV whose type has to be set
 * @param t type to set
 **/
void tlv_set_type(tlv_t *tlv, unsigned char t);

/**
 * Set length of a TLV.
 * @param tlv TLV whose length has to be set
 * @param n length to set
 **/
void tlv_set_length(tlv_t *tlv, unsigned int n);

/**
 * Get the adress of the length of a TLV
 * @param tlv
 * @return a pointer to the length field of tlv
 **/
tlv_t tlv_get_length_ptr(tlv_t *tlv);

/**
 * Get length of a tlv.
 * @param tlv tlv whose length is asked
 * @return value of TLV's length
 **/
unsigned int tlv_get_length(tlv_t *tlv);

/**
 * Get the adress of a tlv value.
 * @param tlv
 * @return a pointer to the value field of tlv
 **/
tlv_t tlv_get_value_ptr(tlv_t *tlv);

/**
 * Write TLV to a buffer
 * @param tlv to write
 * @param data buffer. Must be large enough to contain the tlv
 * @return 0 on succes, -1 on error
 **/
int tlv_mwrite(tlv_t *tlv, void *data);

/**
 * Read the value of a TLV from memory
 * @param tlv tlv prefiled with type and length (will be resized)
 * @param data buffer containing tlv value
 * @return 0 on succes, -1 on error
 **/
int tlv_mread(tlv_t *tlv, char *data);

/**
 * Write a TLV.
 * Offset of {fd} when returning is unspecified.
 * @param tlv tlv to write
 * @param fd file descriptor where tlv is written
 * @return 0 on success, -1 on error
 **/
int tlv_fwrite(tlv_t *tlv, int fd);

/**
 * Read a TLV value.
 * @param tlv tlv to write
 * @param fd file descriptor from which value is read
 * @return 0 on success, -1 on error
 **/
int tlv_fread(tlv_t *tlv, int fd);

/**
 * Write a whole TLV in a file
 * @param tlv TLV to write
 * @param fd file descriptor where you want to dump
 * @return same as write(2)
 **/
int tlv_fdump(tlv_t *tlv, int fd);


/**
 * Write the value of a TLV in a file
 * @param tlv TLV containing the value
 * @param fd file descriptor where you want to dump
 * @return same value as as write(2)
 **/
int tlv_fdump_value(tlv_t *tlv, int fd);

/**
 * Return a string representation for a TLV type
 * @param tlv_type the type
 * @return a string representation of this type
 * @see tlv_str2type
 **/
const char *tlv_type2str(int tlv_type);

/**
 * get a code from a string describing a TLV type
 * @param tlv_type
 * @return code
 * @see tlv_type2str
 **/
char tlv_str2type(char *tlv_type);


/**
 * Import a TLV from a file (or pipe, socket, ...).
 * It supposed to be a valid TLV.
 * @param tlv tlv to fill with imported one.
 * @param fd file descriptor used for reading.
 **/
int tlv_import_from_file(tlv_t *tlv, int fd);

/**
 * Convert a file (or pipe, socket, ...) into a tlv.
 * @param tlv tlv to fill
 * @param fd file descriptor used for reading
 * @param type type of the tlv wanted
 * @param date date to use (if 0, not included in a dated tlv).
 * @return 0 on succes, -1 on error
 */
int tlv_from_file(tlv_t *tlv, int fd, char type, uint32_t date);

/**
 * Return the tlv type for a file from its extension, or NULL if it cannot be
 * determined. The pointer is statically allocated, strdup it if necessary.
 * @param path the path of the file
 * @return the tlv style
 **/
const char *get_tlv_type(const char *path);

/**
 * Create TLV compound with using tlv board value
 * @param tlv_compound is tlv_c
 * @param value
 * @param buff_size
 * @return sizeof new tlv create
 **/
int tlv_create_compound(tlv_t *tlv_c, tlv_t *value, int buff_size);

/**
 * Create TLV dated with using tlv board value
 * @param tlv_dated is tlv_d
 * @param value_tlv is TLV to field to tlv dated
 * @param buff_size
 * @return sizeof new tlv create
 **/
int tlv_create_date(tlv_t *tlv_d, tlv_t *value_tlv, int value_size);

/**
 * Create TLV with using path
 * Return size of new tlv create
 * @param path
 * @param tlv
 * @param type
 * @return sizeof new tlv create
 **/
int tlv_create_path(char *path, tlv_t *tlv, char *type);

/**
 * Create TLV with using input
 * Return size of new tlv create
 * @param tlv
 * @param type
 * @return sizeof new tlv create
 **/
int tlv_create_input(tlv_t *tlv, char *type);

#endif
