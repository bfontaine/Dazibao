#ifndef _WEBUTILS_H
#define _WEBUTILS_H 1

/** @file
 * Utilities for the Web server
 **/

#include "utils.h"
#include <time.h>

/* = Global server properties = */

/**
 * A struct used to store a server's properties
 **/
struct wserver_info {
        /** port the server is listening on **/
        int port;
        /** debug mode (used to serve Pad1s and PadNs) */
        char debug;
        /** hostname of the server */
        char *hostname;
        /** current Dazibao prettified name */
        char *dzname;
        /** path of the current Dazibao */
        char *dzpath;
        /** name of the server */
        char *name;
};
/**
 * A global struct used to store the server's properties
 **/
struct wserver_info WSERVER;

/* = I/O = */

/** The maximum length of a local file path */
#define MAX_FILE_PATH_LENGTH 256

/* = Other helpers = */

/** default extension of a JPEG image */
#define JPEG_EXT ".jpg"
/** default extension of a PNG image */
#define PNG_EXT  ".png"
/** default extension of a GIF image */
#define GIF_EXT  ".gif"
/** default extension of a file */
#define DEFAULT_EXT ""

/**
 * Guess the type of an image TLV from its path. If it ends with .png, it's a
 * TLV_PNG, if it ends with .jpg it's a TLV_JPEG, etc. The function returns -1
 * if the TLV type cannot be found.
 **/
int get_image_tlv_type(const char *path);

/**
 * Return a string representing a given GMT date. 'secs' is the number of
 * seconds since the Epoch, or -2 if you want the current date.
 **/
char *gmtdate(time_t secs);

#endif
