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

/**
 * Return a string representing a given GMT date. 'secs' is the number of
 * seconds since the Epoch, or -2 if you want the current date.
 **/
char *gmtdate(time_t secs);

#endif
