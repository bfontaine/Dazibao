#include "notification-server.h"

static int nbdaz;
static int sock;
static int nbclient = 0;
static int client_max = 10;
static int *client;
static struct config config;

void send_message(int *sock, char *str, int len) {
	printf("[pid:%d] Sending message at client number %d\n", getpid(), *sock);
	
	if (write(*sock, str, len) < len) {
		if (errno == EPIPE) {
			*sock = -1;
			printf("[pid:%d] Client has disconnected: exiting\n", getpid());
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

	for (i = 0; i < client_max; i++) {
		if (client[i] != -1) {
			send_message(&client[i], str, len);
		}
	}

	free(str);
	return (void *)NULL;
}


void *watch_file(void *arg) {

	char *path = (char *)arg;
	struct stat st;
	time_t ctime;
	int sleeping_time = WATCH_SLEEP_DEFAULT;
		
	if (stat(path, &st) == -1) {
		PERROR("stat");
	}

	ctime = st.st_ctime;

	printf("[pid:%d] Started watching %s\n", getpid(), path);	
	while (1) {
		sleep(sleeping_time);
		printf("[pid:%d] Checking %s\n", getpid(), path);	
		if (stat(path, &st) == -1) {
			PERROR("stat");
			continue;
		}
		if (st.st_ctime != ctime) {
			printf("[pid:%d] %s has changed\n", getpid(), path);	
			ctime = st.st_ctime;
			sleeping_time =
				MAX(MIN(sleeping_time / 2, WATCH_SLEEP_DEFAULT),
					WATCH_SLEEP_MIN);
			notify(path);
		} else {
			sleeping_time = MIN(sleeping_time * 1.5,
					WATCH_SLEEP_MAX);
		}
	}
	return (void *)NULL;
}


int nsa(int n, char **file) {
	
	int i;
	int nb = 0;
	for (i = 0; i < n; i++) {
		pthread_t thread;
		pthread_create(&thread, NULL, watch_file, (void *) (file[i]));
		nb++;
	}
	return nb;
}

int set_up_server(char *path) {
	int server;
	struct sockaddr_un saddr;
	
	printf("[pid:%d] Setting up server\n", getpid());

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;

	if (path == NULL) {
		strncpy(saddr.sun_path, getenv("HOME"), 107);
		strncat(saddr.sun_path, "/", 107);
		strncat(saddr.sun_path, ".dazibao-notification-socket", 107);
	} else {
		strncpy(saddr.sun_path, path, 107);
	}

	server = socket(PF_UNIX, SOCK_STREAM, 0);
	
	if(server < 0) {
		perror("socket");
		exit(1);
	}

	if (bind(server, (struct sockaddr*)&saddr, sizeof(saddr))  == -1) {
		if (errno != EADDRINUSE) {
			ERROR("bind", -1);
		}
		if (connect(server, (struct sockaddr*)&saddr,
				sizeof(saddr)) == -1) {
			printf("[pid:%d] Removing old socket at \"%s\"\n",
				getpid(), saddr.sun_path);
			if (unlink(saddr.sun_path) == -1) {
				ERROR("unlink", -1);
			}
			if (bind(server, (struct sockaddr*)&saddr,
					sizeof(saddr))  == -1) {
				ERROR("bind", -1);
			}
		} else {
			if (close(server) == -1) {
				ERROR("close", -1);
			}
			printf("[pid:%d] Socket at \"%s\" already in use.\n",
				getpid(), saddr.sun_path);
			return -1;
		}
	}

	if (listen(server, 10) == -1) {
		ERROR("listen", -1);
	}

	printf("[pid:%d] Socket created at \"%s\"\n", getpid(), saddr.sun_path);

	return server;
}

int accept_client(int server) {

	int pid;
	int s;
	socklen_t len;
	struct sockaddr_un caddr;

	s = accept(server, (struct sockaddr*)&caddr, &len);

	if (s == -1) {
		if (errno == EINTR) {
			sleep(1);
			return accept_client(server);
		}
		ERROR("accept", -1);
	}
	int i;
	for (i = 0; i < client_max; i++) {
		if (client[i] == -1) {
			client[i] = s;
			break;
		}
	}
	printf("[pid:%d] *** New client connected and attached to %d ***\n", getpid(), s);
	return s;

}

int main(int argc, char **argv) {

	char *path = NULL;
	char **files = NULL;
	int server;
	int next_arg = 1;
	int client_max = 10;

	if (argc < 2) {
                printf("Usage:\n\t%s [OPTION] [FILE]\n", argv[0]);
                exit(EXIT_FAILURE);
	}

	while (files == NULL) {
		if (strcmp(argv[next_arg], "--path") == 0) {
			if (next_arg > argc - 2) {
				printf("Usage:\n\t%s [OPTION] [FILE]\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			path = argv[next_arg + 1];
			next_arg += 2;			
		} else if (strcmp(argv[next_arg], "--max") == 0) {
			if (next_arg > argc - 2) {
				printf("Usage:\n\t%s [OPTION] [FILE]\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			client_max = atoi(argv[next_arg + 1]);
			next_arg += 2;			
		} else {
			files = &argv[next_arg];
			nbdaz = argc - next_arg;
		}
	}


	client = mmap(NULL, sizeof(*client)*client_max, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	memset(client, -1, sizeof(*client)*client_max);

	server = set_up_server(path);

	if (server == -1) {
		ERROR("set_up_server", -1);
	}

	printf("[pid:%d] Server set up\n", getpid());	

	if(nsa(nbdaz, files) != nbdaz) {
		fprintf(stderr, "[pid:%d] Some files could not be watched\n", getpid());
	} else {
		printf("[pid:%d] nsa launch has gone well\n", getpid());	
	}
	
	while (1) {
		if (accept_client(server) > 0) {
			nbclient++;
			printf("[pid:%d] Now handles %d clients\n", getpid(), nbclient);	
		} else {
			PERROR("accept_client");
			continue;
		}
	}

	munmap(client, sizeof(*client)*client_max);

	return 0;
}
