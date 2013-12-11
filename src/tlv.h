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

/** test that a TLV type is valid */
#define TLV_VALID_TYPE(type) (0 < (type) && (type) <= 6)

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

/** Known tlv extention types **/
static const char *tlv_types_ext[][2] = {
        /* type text */
        { "html" , "2"},
        { "css"  , "2"},
        { "js"   , "2"},
        { "csv"  , "2"},
        { "gif"  , "2"},
        { "gzip" , "2"},
        { "json" , "2"},
        { "pdf"  , "2"},
        { "rss"  , "2"},
        { "svg"  , "2"},
        { "tiff" , "2"},
        { "xml"  , "2"},
        { "zip"  , "2"},
        { "md"   , "2"}, /* Markdown */
        { "txt"  , "2"},
        /* typt png */
        { "png"  , "3"},
        /* typt jpg */
        { "jpeg" , "4"},
        { "jpg"  , "4"},
};

/** Number of registered tlv types **/
#define TLV_TYPES_COUNT (sizeof(tlv_types_ext)/sizeof(char*[2]))
/** arbitrary TLV depth limit. This is NOT a hard limit, but some functions
 * need to have a limit */
#define TLV_MAX_DEPTH 16

/**
 * The type of a TLV
 **/
typedef char* tlv_t;

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
int tlv_get_type(tlv_t tlv);

/**
 * Set the type of a TLV
 * @param tlv TLV whose type has to be set
 * @param t type to set
 **/
void tlv_set_type(tlv_t *tlv, char t);

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
tlv_t tlv_get_length_ptr(tlv_t tlv);

/**
 * Get length of a tlv.
 * @param tlv tlv whose length is asked
 * @return value of TLV's length
 **/
unsigned int tlv_get_length(tlv_t tlv);

/**
 * Get the adress of a tlv value.
 * @param tlv
 * @return a pointer to the value field of tlv
 **/
tlv_t tlv_get_value_ptr(tlv_t tlv);

/**
 * Write a TLV.
 * Offset of {fd} when returning is unspecified.
 * @param tlv tlv to write
 * @param fd file descriptor where tlv is written
 * @return 0 on success, -1 on error
 **/
int tlv_write(tlv_t tlv, int fd);

/**
 * Read a TLV value.
 * @param tlv tlv to write
 * @param fd file descriptor from which value is read
 * @return 0 on success, -1 on error
 **/
int tlv_read(tlv_t *tlv, int fd);

/**
 * Write a whole TLV in a file
 * @param tlv TLV to write
 * @param fd file descriptor where you want to dump
 * @return same as write(2)
 **/
int dump_tlv(tlv_t tlv, int fd);


/**
 * Write the value of a TLV in a file
 * @param tlv TLV containing the value
 * @param fd file descriptor where you want to dump
 * @return same value as as write(2)
 **/
int dump_tlv_value(tlv_t tlv, int fd);

/**
 * Return a string representation for a TLV type
 * @param tlv_type the type
 * @return a string representation of this type
 **/
const char *tlv_type2str(char tlv_type);

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
 * Create TLV with using path
 * Return size of new tlv create
 * @param path
 * @param tlv
 * @return sizeof new tlv create
 **/
int tlv_create_path(char *path, tlv_t *tlv);

/**
 * Create TLV with using daz for tlv type compound
 * Return size of value tlv compound
 * @param path
 * @param tlv
 * @return sizeof new tlv create
 **/
int tlv_create_daz(char *daz, tlv_t *tlv);

#endif
