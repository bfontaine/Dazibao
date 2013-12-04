#ifndef _WEBUTILS_H
#define _WEBUTILS_H 1

#include "utils.h"
#include <time.h>

/* = Global server properties = */
struct wserver_info {
        int port;
        char debug;    /* Debug mode */
        char *hostname;
        char *dzname;  /* Dazibao "pretty" name */
        char *dzpath;  /* Dazibao path */
        char *name;    /* Name of the server */
} WSERVER;

/* = I/O = */

#define MAX_FILE_PATH_LENGTH 256

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

/**
 * Return a string representing a given GMT date. 'secs' is the number of
 * seconds since the Epoch, or -2 if you want the current date.
 **/
char *gmtdate(time_t secs);

#endif
