#ifndef _WEBUTILS_H
#define _WEBUTILS_H 1

/* = Global server properties = */
struct wserver_info {
        int port;
        char *hostname;
        char *dzname;
} WSERVER;

/* = I/O = */

/**
 * Wrapper around write(2) to write the whole buffer instead of (sometimes)
 * only a part of it.
 **/
int write_all(int fd, char *buff, int len);

/* = Other helpers = */

#define JPEG_EXT ".jpg"
#define PNG_EXT  ".png"
#define DEFAULT_EXT ""

/**
 * Guess the type of an image TLV from its path. If it ends with .png, it's a
 * TLV_PNG, if it ends with .jpg it's a TLV_JPEG. The function returns -1 if
 * the TLV type cannot be found.
 **/
int get_image_tlv_type(const char *path);

#endif
