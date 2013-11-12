#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "utils.h"

#define NSA_WAIT_TIME 30

/**
 * Set up the server
 * @return file descriptor (positive integer) of server on success
 * @return -1 on error
 */
int set_up_server(void);

/**
 * Establish connection with client, and create a new process to handle notifications
 * @param server file descriptor of server used
 * @return pid of client on success
 * @return -1 on error
 */
int accept_client(int server);

/**
 * @return -1 on error
 */
int nsa(int nb, char **file);

int main(int argc, char **argv);
