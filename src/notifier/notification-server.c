#include "notification-server.h"

static struct ns_config conf;
int _log_level;

void send_message(int s_index, char *str, int len) {

	LOGDEBUG("Sending message at client n°%d", s_index);
	
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
	int status;

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
	
	hash = qhashmurmur3_32(buf, st.st_size);

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
				MAX(MIN(sleeping_time / 2, conf.w_sleep_default),
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
	for (i = 0; i < conf.nb_files; i++) {
		pthread_t thread;
		pthread_create(&thread, NULL, watch_file, (void *) (conf.file[i]));
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
		strncpy(saddr.sun_path, getenv("HOME"), 107);
		strncat(saddr.sun_path, "/", 107);
		strncat(saddr.sun_path, ".dazibao-notification-socket", 107);
	} else {
		strncpy(saddr.sun_path, conf.s_path, 107);
	}

	conf.s_socket = socket(PF_UNIX, SOCK_STREAM, 0);
	
	if(conf.s_socket < 0) {
		PERROR("socket");
		exit(1);
	}

	if (bind(conf.s_socket, (struct sockaddr*)&saddr, sizeof(saddr))  == -1) {
		if (errno != EADDRINUSE) {
			ERROR("bind", -1);
		}
		if (connect(conf.s_socket, (struct sockaddr*)&saddr,
				sizeof(saddr)) == -1) {
			LOGINFO("Removing old socket at \"%s\"", saddr.sun_path);
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
			LOGERROR("Socket at \"%s\" already in use", saddr.sun_path);
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

	int next_arg = 1;

	conf.client_max = MAX_CLIENTS;
	conf.w_sleep_min = WATCH_SLEEP_MIN;
	conf.w_sleep_default = WATCH_SLEEP_DEFAULT;
	conf.w_sleep_max = WATCH_SLEEP_MAX;
	conf.reliable = RELIABLE_DEFAULT;

	while (next_arg < argc) {
		if (strcmp(argv[next_arg], "--path") == 0) {
			if (next_arg > argc - 3) {
				LOGFATAL("\"--path\" parameter or [FILE] is missing");
				return -1;
			}
			conf.s_path = argv[next_arg + 1];
			next_arg += 2;			
		} else if (strcmp(argv[next_arg], "--max") == 0) {
			if (next_arg > argc - 3) {
				LOGFATAL("\"--max\" parameter or [FILE] is missing");
				return -1;
			}
			conf.client_max = strtol(argv[next_arg + 1], NULL, 10);
			next_arg += 2;
		} else if (strcmp(argv[next_arg], "--wtimemin") == 0) {
			if (next_arg > argc - 3) {
				LOGFATAL("\"--wtimemin\" parameter or [FILE] is missing");
				return -1;
			}
			conf.w_sleep_min = strtol(argv[next_arg + 1], NULL, 10);
			next_arg += 2;
		} else if (strcmp(argv[next_arg], "--wtimemax") == 0) {
			if (next_arg > argc - 3) {
				LOGFATAL("\"--wtimemax\" parameter or [FILE] is missing");
				return -1;
			}
			conf.w_sleep_max = strtol(argv[next_arg + 1], NULL, 10);
			next_arg += 2;
		} else if (strcmp(argv[next_arg], "--wtimedef") == 0) {
			if (next_arg > argc - 3) {
				LOGFATAL("\"--wtimedef\" parameter or [FILE] is missing");
				return -1;
			}
			conf.w_sleep_default = strtol(argv[next_arg + 1], NULL, 10);
			next_arg += 2;
		} else if (strcmp(argv[next_arg], "--reliable") == 0) {
			if (next_arg > argc - 2) {
				LOGFATAL("[FILE] is missing");
				return -1;
			}
			conf.reliable = 1;
			next_arg += 1;			
		} else {
			conf.file = &argv[next_arg];
			conf.nb_files = argc - next_arg;
			break;
		}
	}

	return 0;
}


int main(int argc, char **argv) {

	int client_max = 10;
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

	conf.c_socket = malloc(sizeof(*conf.c_socket) * client_max);
	
	if (conf.c_socket == NULL){
		ERROR("malloc", -1);
	}
	
	memset(conf.c_socket, -1, sizeof(*conf.c_socket) * conf.client_max);

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
