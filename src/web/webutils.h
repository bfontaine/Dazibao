#ifndef _WEBUTILS_H
#define _WEBUTILS_H 1

#include "utils.h"

/* = Global server properties = */
struct wserver_info {
        int port;
        char debug;     /* Debug mode */
        char *hostname;
        char *dzname;
} WSERVER;

#define WLOGDEBUG(fmt, ...) _LOG(LOG_LVL_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define WLOGINFO(fmt, ...)  _LOG(LOG_LVL_INFO,  "INFO", fmt, ##__VA_ARGS__)
#define WLOGWARN(fmt, ...)  _LOG(LOG_LVL_WARN,  "WARN", fmt, ##__VA_ARGS__)
#define WLOGERROR(fmt, ...) _LOG(LOG_LVL_ERROR, "ERROR", fmt, ##__VA_ARGS__)
#define WLOGFATAL(fmt, ...) _LOG(LOG_LVL_FATAL, "FATAL", fmt, ##__VA_ARGS__)

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
