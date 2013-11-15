#ifndef _DAZIWEB_H
#define _DAZIWEB_H 1

#include "utils.h"
#include "web/request.h"

#define DEFAULT_PORT 3437
#define MAX_QUEUE 64

/**
 * Handler for signals used to close the listening socket.
 **/
void clean_close();

#endif
