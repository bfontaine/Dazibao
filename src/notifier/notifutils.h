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

#endif
