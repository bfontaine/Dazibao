#ifndef _NOTIFICATION_SERVER_H
#define _NOTIFICATION_SERVER_H 1

#include <time.h>
#include <sys/types.h>

/** @file
 * A notifications server
 */

#ifndef MAP_ANONYMOUS
/**
 * MAP_ANONYMOUS is undefined on Mac OS X
 * but MAP_ANON (which is deprecated by POSIX) is
 */
#define MAP_ANONYMOUS MAP_ANON
#endif

#define MAX_CLIENTS 10
#define RELIABLE_DEFAULT 1
#define WATCH_SLEEP_MIN 2
#define WATCH_SLEEP_DEFAULT 10
#define WATCH_SLEEP_MAX 60

#define NS_ERR_FULL "EServer is full"


/** general config of notification server */
struct ns_config {

        /** max number of clients */
        int client_max;
        /** number of files watched by server */
        int nb_files;
        /** path of the socket used to accept clients */
        char *s_path;
        /** pathes of files watched */
        char **file;
        /** flag enabling or disabling reliable mode */
        char reliable;

        /* sockets */
        /** file descriptor of server */
        int s_socket;
        /** file descriptors of clients */
        int *c_socket;
	pthread_mutex_t *c_mtx;

        /* waiting time */
        /** minimum interval between two file check */
        int w_sleep_min;
        /** default interval between two file check */
        int w_sleep_default;
        /** maximum interval between two file check */
        int w_sleep_max;
};


static struct ns_config conf;
int _log_level;

/**
 * Send a message to a client.
 * If client disconnected, remove it from client list (c_socket)
 * @param s_index index of client in c_socket
 * @param str message to send
 * @param len length of message
 */
void send_message(int s_index, char *str, int len);

/**
 * Send a notify to all clients connected to the server
 * @param arg path of the file which changed
 */
void *notify(void *arg);

/**
 * Look for a change in 'ctime' of a file
 * @param file path of file
 * @param old_time previous recorded ctime of file (or 0 for initialization)
 * @return 0 if file did not change or on initialization
 * @return 1 if file changed
 * @return -1 on error
 */
int unreliable_watch(char *file, time_t *old_time);


/**
 * Look for a change in hashcode of a file
 * @param file path of file
 * @param old_hash previous recorded hashcode of file (or 0 for initialization)
 * @return 0 if file did not change or on initialization
 * @return 1 if file changed
 * @return -1 on error
 */
int reliable_watch(char *file, uint32_t *old_hash);

/**
 * Loop periodicly watching for file change
 * @param path file to watch
 * @return (void *)NULL on error (never return if success)
 */
void *watch_file(void *path);

/**
 * Launch a thread for each file in conf using watch_file
 * @see watch_file
 * @return number of files which are being watched on success
 * @return -1 on error
 */
int nsa(void);

/**
 * Set up the server (socket, bind, ...) and update 'conf'
 * @return 0 on success
 * @return -1 on error
 */
int set_up_server(void);

/**
 * If server is not full, wait for a connexion and update client list in conf
 * @return 0 on success
 * @return -1 on error
 */
int accept_client(void);

/**
 * Parse arguments and update 'conf' with found options
 * @param argc number of arguments in argv
 * @param argv arguments to parse
 * @return 0 on success
 * @return -1 on error
 */
int parse_arg(int argc, char **argv);
#endif
