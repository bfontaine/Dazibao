#ifndef _NOTIFUTILS_H
#define _NOTIFUTILS_H 1

/** @file
 * Utilities for the notifications server
 */

#include <stdint.h>
#include <stddef.h>
#include <sys/un.h>

#ifndef UNIX_PATH_MAX
/** defined for systems that don't have it */
struct sockaddr_un sizecheck;
/** defined for systems that don't have it */
#define UNIX_PATH_MAX sizeof(sizecheck.sun_path)
#endif

#endif
