#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "utils.h"

#define WATCH_SLEEP_MIN 2
#define WATCH_SLEEP_DEFAULT 10
#define WATCH_SLEEP_MAX 60

struct file_watcher {
	int pid;
	char *file;
};

/**
 * Compare two file_watcher struct
 */
int fwcmp (const void*, const void*);

/**
 * Signal handler.
 * Write on client socket when SIGUSR1 is received.
 */
void notify(int sigrtmin, siginfo_t *info, void *unused_ptr);

/**
 * Create a new process to check file changes.
 * @param path path of file to check
 * @return pid of new process, in father process.
 * @return never return in child process.
 * @return -1 on error.
 */
int watch_file(char *path);

/**
 * Set up the server
 * @return file descriptor (positive integer) of server on success
 * @return -1 on error
 */
int set_up_server(char *);

/**
 * Establish connection with client, and create a new process to handle
 * notifications
 * @param server file descriptor of server used
 * @return pid of client on success
 * @return -1 on error
 */
int accept_client(int server);

/**
 * @return -1 on error
 */
int nsa(int nb, char **file);
