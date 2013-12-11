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
#include <time.h>
#include <fcntl.h>
#include "utils.h"
#include "notifutils.h"
#include "hash.h"
#include "notification-server.h"

static struct ns_config conf;
int _log_level;

void send_message(int s_index, char *str, int len) {

        if (write(conf.c_socket[s_index], str, len) < len) {
                if (errno == EPIPE) {
                        conf.c_socket[s_index] = -1;
                        LOGINFO("Client disconnected");
                } else {
                        PERROR("write");
                }
        }
}

void *notify(void *arg) {
        char *file = (char *)arg;
        int len = strlen(file) + 2;
        char *str = malloc(sizeof(*str) * len);
        int i;
        str[0] = 'C';
        memcpy(&str[1], file, len - 2);
        str[len - 1] = '\n';

        for (i = 0; i < conf.client_max; i++) {
                if (conf.c_socket[i] != -1) {
                        send_message(i, str, len);
                }
        }

        free(str);
        return (void *)NULL;
}

int unreliable_watch(char *file, time_t *old_time) {

        struct stat st;

        if (stat(file, &st) == -1) {
                PERROR("stat");
                return -1;
        }

        if (*old_time == 0) {
                *old_time = st.st_ctime;
                return 0;
        }

        if (st.st_ctime != *old_time) {
                LOGINFO("%s changed", file);
                *old_time = st.st_ctime;
                return 1;
        }

        return 0;
}

int reliable_watch(char *file, uint32_t *old_hash) {

        struct stat st;
        uint32_t hash;
        char *buf;
        int fd = open(file, O_RDONLY);
        int status = 0;

        if (fd < 0) {
                ERROR("open", -1);
        }

        if (fstat(fd, &st) < 0) {
                PERROR("fstat");
                status = -1;
                goto CLOSE;
        }

        buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (buf == MAP_FAILED) {
                PERROR("mmap");
                goto CLOSE;
        }

        hash = SuperFastHash(buf, st.st_size);

        if (*old_hash == 0) {
                *old_hash = hash;
                status = 0;
        }

        if (hash != *old_hash) {
                *old_hash = hash;
                status = 1;
        } else {
                status = 0;
        }

        if (munmap(buf, st.st_size) == -1) {
                PERROR("munmap");
        }

CLOSE:
        if (close(fd) == -1) {
                PERROR("close");
        }

        return status;
}

void *watch_file(void *arg) {

        char *path = (char *)arg;

        union {
                time_t time;
                uint32_t hash;
        } value;

        memset(&value, 0, sizeof(value));

        int sleeping_time = conf.w_sleep_default;
        int changed = 0;

        LOGINFO("Started watching %s", path);

        while (1) {
                LOGDEBUG("watching %s", path);
                if (conf.reliable) {
                        changed = reliable_watch(path, &(value.hash));
                } else {
                        changed = unreliable_watch(path, &(value.time));
                }

                if (changed == 1) {
                        sleeping_time =
                                MAX(MIN(sleeping_time / 2,
                                                        conf.w_sleep_default),
                                        conf.w_sleep_min);
                        notify(path);
                } else if (changed == 0) {
                        sleeping_time =
                                MIN(sleeping_time * 1.5, conf.w_sleep_max);
                } else {
                        LOGWARN("Failed checking %s", path);
                        continue;
                }
                sleep(sleeping_time);
        }

        return (void *)NULL;
}


int nsa() {
        int i;
        pthread_attr_t attr;

        if (pthread_attr_init(&attr) != 0) {
                ERROR("pthread_attr_init", -1);
        }

        if (pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) != 0) {
                ERROR("pthread_attr_setdetachstate", -1);
        }

        for (i = 0; i < conf.nb_files; i++) {
                pthread_t thread;
                if (pthread_create(&thread, &attr, watch_file,
                                (void *) (conf.file[i])) != 0) {
                        PERROR("pthread_create");
                }
        }

        if (pthread_attr_destroy(&attr) != 0) {
                PERROR("pthread_attr_destroy");
        }

        return 0;
}

int set_up_server() {

        struct sigaction action;
        struct sockaddr_un saddr;

        LOGDEBUG("Setting up server");

        action.sa_handler = SIG_IGN;
        sigfillset(&action.sa_mask);

        if (sigaction(SIGPIPE, &action, 0)) {
                ERROR("sigaction", -1);
        }

        memset(&saddr, 0, sizeof(saddr));
        saddr.sun_family = AF_UNIX;

        if (conf.s_path == NULL) {
                strncpy(saddr.sun_path, getenv("HOME"), UNIX_PATH_MAX - 1);
                strncat(saddr.sun_path, "/", UNIX_PATH_MAX - 1);
                strncat(saddr.sun_path, ".dazibao-notification-socket",
                                UNIX_PATH_MAX - 1);
        } else {
                strncpy(saddr.sun_path, conf.s_path, UNIX_PATH_MAX - 1);
        }

        conf.s_socket = socket(PF_UNIX, SOCK_STREAM, 0);

        if(conf.s_socket < 0) {
                PERROR("socket");
                exit(1);
        }

        if (bind(conf.s_socket, (struct sockaddr*)&saddr,
                                sizeof(saddr))  == -1) {
                if (errno != EADDRINUSE) {
                        ERROR("bind", -1);
                }
                if (connect(conf.s_socket, (struct sockaddr*)&saddr,
                                sizeof(saddr)) == -1) {
                        LOGINFO("Removing old socket at \"%s\"",
                                        saddr.sun_path);
                        if (unlink(saddr.sun_path) == -1) {
                                ERROR("unlink", -1);
                        }
                        if (bind(conf.s_socket, (struct sockaddr*)&saddr,
                                        sizeof(saddr))  == -1) {
                                ERROR("bind", -1);
                        }
                } else {
                        if (close(conf.s_socket) == -1) {
                                ERROR("close", -1);
                        }
                        LOGERROR("Socket at \"%s\" already in use",
                                        saddr.sun_path);
                        return -1;
                }
        }

        if (listen(conf.s_socket, 10) == -1) {
                ERROR("listen", -1);
        }

        LOGINFO("Socket created at \"%s\"", saddr.sun_path);

        return 0;
}

int accept_client() {

        struct sockaddr_un caddr;
        socklen_t len = sizeof(caddr);
        int i;

        for (i = 0; i < conf.client_max; i++) {
                if (conf.c_socket[i] == -1) {
                        conf.c_socket[i] =
                                accept(conf.s_socket,
                                        (struct sockaddr*)&caddr,
                                        &len);
                        if (conf.c_socket[i] == -1) {
                                if (errno == EINTR) {
                                        sleep(1);
                                        return accept_client();
                                }
                                ERROR("accept", -1);
                        }
                        LOGINFO("New client connected");
                        break;
                }
        }

        if (i == conf.client_max) {
                LOGWARN("Server is full");
                return -1;
        }

        return 0;
}


int parse_arg(int argc, char **argv) {

        LOGDEBUG("begin parsing args");

        conf.client_max = MAX_CLIENTS;
        conf.w_sleep_min = WATCH_SLEEP_MIN;
        conf.w_sleep_default = WATCH_SLEEP_DEFAULT;
        conf.w_sleep_max = WATCH_SLEEP_MAX;
        conf.reliable = RELIABLE_DEFAULT;

        struct s_option options[] = {
                {"--path", ARG_TYPE_STRING, (void *)conf.s_path},
                {"--max", ARG_TYPE_STRING, (void *)&(conf.client_max)},
                {"--wtimemin", ARG_TYPE_INT, (void *)&(conf.w_sleep_min)},
                {"--wtimemax", ARG_TYPE_INT, (void *)&(conf.w_sleep_max)},
                {"--wtimedef", ARG_TYPE_INT, (void *)&(conf.w_sleep_default)},
                {"--reliable", ARG_TYPE_INT, (void *)&(conf.reliable)},
        };

        struct s_args args = {
                &(conf.nb_files), &conf.file, options
        };

        if (jparse_args(argc, argv, &args, 6) != 0) {
                ERROR("parse_args", -1);
        }

        return 0;
}


int main(int argc, char **argv) {

	int i;

        _log_level = LOG_LVL_DEBUG;

        memset(&conf, 0, sizeof(conf));

        if (argc < 2) {
                LOGFATAL("Usage:\n\t%s [OPTION] [FILE]", argv[0]);
                exit(EXIT_FAILURE);
        }

        if (parse_arg(argc, argv) == -1) {
                LOGFATAL("Wrong arguments, see documentation for details");
                exit(EXIT_FAILURE);
        }

        conf.c_socket = malloc(sizeof(*conf.c_socket) * conf.client_max);

        if (conf.c_socket == NULL) {
                ERROR("malloc", -1);
        }

	conf.c_mtx = malloc(sizeof(*conf.c_mtx) * conf.client_max);

        if (conf.c_mtx == NULL) {
                ERROR("malloc", -1);
        }

	for (i = 0; i < conf.client_max; i++) {
		conf.c_socket[i] = -1;
		conf.c_mtx[i] = PTHREAD_MUTEX_INITIALIZER;
	}

        if (set_up_server() == -1) {
                PERROR("set_up_server");
                goto OUT;
        }

        LOGINFO("Server set up");

        if(nsa() != 0) {
                PERROR("nsa");
                goto OUT;
        }

        while (1) {
                if (accept_client() < 0) {
                        PERROR("accept_client");
                        continue;
                }
        }
OUT:
        free(conf.c_socket);

        return 0;
}
