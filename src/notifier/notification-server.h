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
#include <sys/mman.h>
#include <pthread.h>
#include <fcntl.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#include "utils.h"
#include "notifutils.h"

#define MAX_CLIENTS 10
#define RELIABLE_DEFAULT 1
#define WATCH_SLEEP_MIN 2
#define WATCH_SLEEP_DEFAULT 10
#define WATCH_SLEEP_MAX 60


struct ns_config {
	
	/* general config */
	int client_max;
	int nb_files;
	char *s_path;
	char **file;
	char reliable;
	
	/* sockets */
	int s_socket;
	int *c_socket;
	
	/* waiting time */
	int w_sleep_min;
	int w_sleep_default;
	int w_sleep_max;
};

void send_message(int s_index, char *str, int len);

void *notify(void *arg);

int unreliable_watch(char *file, time_t *old_time);

int reliable_watch(char *file, uint32_t *old_hash);

void *watch_file(void *path);

int nsa(void);

int set_up_server(void);

int accept_client(void);

int parse_arg(int argc, char **argv);
