#include "notification-server.h"

static char **filename;
static int *pids;
static int nbdaz;
static int sock;
static int nbclient = 0;

/**
 * TODO:
 * - queue signals
 */
void notify(int unused_sigint, siginfo_t *info, void *unused_ptr) {

	printf("[pid:%d] Received SIGUSR1: %d\n", getpid(), unused_sigint == SIGUSR1);

	int i;
	for (i = 0; i < nbdaz; i++) {
		if (pids[i] == info->si_pid) {
			break;
		}
	}
	if (i < nbdaz) {
		int len = strlen(filename[i]) + 2;
		char *str = malloc(sizeof(*str) * len);
		str[0] = 'C';
		memcpy(&str[1], filename[i], len - 2);
		str[len - 1] = '\n';
		if (write(sock, str, len) < (int)(strlen(filename[i]) + 2)) {
			if (errno == EPIPE) {
				printf("[pid:%d] Client has disconnected: exiting\n", getpid());
				exit(EXIT_SUCCESS);
			} else {
				PERROR("write");
			}
		}
		free(str);
	} else {
		printf("[pid:%d] Received signal from unkown process: %d\n",
			getpid(), info->si_pid);
	}
}


int watch_file(char *path) {
	
	/**
	 * TODO:
	 * - use inotify on linux systems ?
	 * - the way we check changes could probably be improved
	 * - should parse file to see changes ?
	 */

	int pid = fork();

	if (pid < -1) {
		ERROR("fork", -1);
		
	} else if (pid == 0) {

		struct stat st;
		time_t ctime;
		int sleeping_time = WATCH_SLEEP_DEFAULT;

		/* watchers ignore signals USER1 */
		struct sigaction action;
		action.sa_flags = SA_RESTART;
		action.sa_handler = SIG_IGN;
		
		if(sigaction(SIGUSR1, &action, NULL) == -1) {
			ERROR("sigaction", -1);
		}

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
				if (kill(0, SIGUSR1) == -1) {
					PERROR("kill");
					continue;
				}
				ctime = st.st_ctime;
				sleeping_time =
					MAX(MIN(sleeping_time / 2, WATCH_SLEEP_DEFAULT),
						WATCH_SLEEP_MIN);
					
			} else {
				sleeping_time = MIN(sleeping_time * 1.5,
						WATCH_SLEEP_MAX);
			}
		}
	} else {
		return pid;
	}
}


int nsa(int n, char **file) {
	
	int i;
	int nb = 0;
	for (i = 0; i < n; i++) {
		printf("[pid:%d] Launching %s watch\n", getpid(), file[i]);	
		pids[i] = watch_file(file[i]);
		if (pids[i] != -1) {
			nb++;
		}
	}
	return nb;
}

int set_up_server(void) {
	printf("[pid:%d] Setting up server\n", getpid());	
	int server;
	struct sockaddr_un saddr;

	/* set up the server adress */	
	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strncpy(saddr.sun_path, getenv("HOME"), 107);
	strncat(saddr.sun_path, "/", 107);
	strncat(saddr.sun_path, ".dazibao-notification-socket", 107);
	
	server = socket(PF_UNIX, SOCK_STREAM, 0);
	if(server < 0) {
		perror("socket");
		exit(1);
	}
	
	if (unlink(saddr.sun_path) == -1 && errno != ENOENT) {
		ERROR("unlink", -1);
	}

	if (bind(server, (struct sockaddr*)&saddr, sizeof(saddr))  == -1) {
		ERROR("bind", -1);
	}

	if (listen(server, 10) == -1) {
		ERROR("listen", -1);
	}

	return server;
}

int accept_client(int server) {

	int pid;
	int client;
	socklen_t len;
	struct sockaddr_un caddr;

	client = accept(server, (struct sockaddr*)&caddr, &len);

	if (client == -1) {
		if (errno == EINTR) {
			/* manual restart */
			return accept_client(server);
		}
		ERROR("accept", -1);
	}
	printf("[pid:%d] *** New client connected ***\n", getpid());

	pid = fork();
	if (pid < 0){
		ERROR("fork", -1);
	} else if (pid == 0) {
		sock = client;

		/* set handler for SIGUSR1 */
		struct sigaction action;
		action.sa_flags = SA_SIGINFO;
		action.sa_sigaction = notify;
		sigfillset(&action.sa_mask);
		if(sigaction(SIGUSR1, &action, NULL) == -1) {
			/* TODO: delete client */
			ERROR("sigaction", -1);
		}
		printf("[pid:%d] Client configured\n", getpid());	

		while (1) {
			pause();
		}
		return 0;
	} else {

		return pid;
	}
}



void collect_zombie(int unused_sigint, siginfo_t *info, void *unused_ptr) {
	if (waitpid(info->si_pid, NULL, 0) == -1) {
		PERROR("waitpid");
	} else {
		nbclient--;
		printf("[pid:%d] Client in activity: %d\n", getpid(), nbclient);	
	}
}

int main(int argc, char **argv) {

	/*
	 * TODO:
	 * - define signal handler to notify children when the file changed
	 * - wait for child before leaving ?
	 */

	int server;

	if (argc < 2) {
                printf("Usage:\n\t%s <dazibao1> <dazibao2> ... <dazibaon>\n",
                                argv[0]);
                exit(EXIT_FAILURE);
	}

	nbdaz = argc - 1;

	filename = &argv[1];

	pids = malloc(sizeof(*pids) * nbdaz);	

	server = set_up_server();
	if (server == -1) {
		ERROR("set_up_server", -1);
	}
	printf("[pid:%d] Server set up\n", getpid());	

	/* ignore signal used by notifier */
	struct sigaction action;
	action.sa_flags = SA_RESTART;
	action.sa_handler = SIG_IGN;
	
	if(sigaction(SIGUSR1, &action, NULL) == -1) {
		ERROR("sigaction", -1);
	}

		/* set handler for SIGUSR1 */
	struct sigaction action2;
	action2.sa_flags = SA_SIGINFO | SA_RESTART;
	action2.sa_sigaction = collect_zombie;
	if(sigaction(SIGCHLD, &action2, NULL) == -1) {
		ERROR("sigaction", -1);
	}

	printf("[pid:%d] sigaction set\n", getpid());	

	if(nsa(argc - 1, &argv[1]) != argc - 1) {
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

	free(pids);

	return 0;
}
