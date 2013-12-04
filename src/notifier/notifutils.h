#ifndef _NOTIFUTILS_H
#define _NOTIFUTILS_H 1

/** @file
 * Utilities for the notifications server
 **/

#include <stdint.h>
#include <stddef.h>

uint32_t qhashmurmur3_32(const void *data, size_t nbytes);

#endif
