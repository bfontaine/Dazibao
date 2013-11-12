#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "utils.h"

/**
 * Set up the server
 * @return file descriptor (positive integer) of server on success
 * @return -1 on error
 */
int set_up_server();

/**
 * Establish connection with client, and create a new process to handle notifications
 * @param server file descriptor of server used
 * @return pid of client on success
 * @return -1 on error
 */
int accept_client(int server);


int main(int argc, char **argv);
