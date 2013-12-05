#ifndef _NOTIFUTILS_H
#define _NOTIFUTILS_H 1

/** @file
 * Utilities for the notifications server
 */

#include <stdint.h>
#include <stddef.h>
#include <sys/un.h>

#ifndef UNIX_PATH_MAX
struct sockaddr_un sizecheck;
#define UNIX_PATH_MAX sizeof(sizecheck.sun_path)
#endif

/**
 * @param data data to be hashed
 * @param nbytes number of bytes actually hashed (e.g. size of data)
 * @return hashcode
 */
uint32_t qhashmurmur3_32(const void *data, size_t nbytes);

#endif
