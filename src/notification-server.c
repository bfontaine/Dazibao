#include "notification-server.h"


int nsa(int n, char **file) {
	
	/*
	 * TODO:
	 * - the way we check changes could probably be improved
	 */

	struct stat st;
	time_t *mtime;
	int i;

	mtime = malloc(n * sizeof(*mtime));

	if (mtime == NULL) {
		PERROR("malloc");
		goto OUT;
	}

	for (i = 0; i < n; i++) {
		if (stat(file[i], &st) == -1) {
			PERROR("stat");
			goto OUT;
		}
		mtime[i] = st.st_mtime;
	}

	while (1) {
		sleep(NSA_WAIT_TIME);

		for (i = 0; i < n; i++) {
			if (stat(file[i], &st) == -1) {
				PERROR("stat");
				continue;
			}
			if (st.st_mtime != mtime[i]) {
				/* send signal to group, providing index i */
				mtime[i] = st.st_mtime;
			}
		}
	}

	OUT:
	free(mtime);
	return -1;
}

int set_up_server(void) {
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
		ERROR("accept", -1);
	} 
	
	pid = fork();
	if (pid < 0){
		ERROR("fork", -1);
	} else if (pid == 0) {
		/*
		 * TODO:
		 * - set signal handler
		 * - wait for notification
		 */
		return 0;
	} else {
		return pid;
	}
}

int main(int argc, char **argv) {

	/* 
	 * TODO:
	 * - fork to watch dazibao (before "accept loop")
	 * - define signal handler to notify children when the file changed
	 * - wait for child before leaving ?
	 */

	int server;
	int nbclient = 0;
	int pid;

	if (argc < 2) {
                printf("Usage:\n\t%s <dazibao1> <dazibao2> ... <dazibaon>\n", argv[0]);
                exit(EXIT_FAILURE);
	}
	server = set_up_server();
	if (server == -1) {
		ERROR("set_up_server", -1);
	}

	pid = fork();
	
	if (pid == -1) {
		
	} else if (pid == 0) {
		if(nsa(argc - 1, &argv[1]) == -1) {
			ERROR("nsa", -1);
		}
	}

	while (1) {
		if (accept_client(server) > 0) {
			nbclient++;
		} else {
			PERROR("accept_client");
			continue;
		}
	}

	return 0;
}
