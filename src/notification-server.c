#include "notification-server.h"

int set_up_server() {
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
		PERROR("accept");
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
	 * - parse args and verify dazibao provided
	 * - fork to watch dazibao (before "accept loop")
	 * - define signal handler to notify children when the file changed
	 * - wait for child before leaving ?
	 */

	int server;

	if (argc < 2) {
                printf("Usage:\n\t%s <dazibao>\n", argv[0]);
                exit(EXIT_FAILURE);
	}

	if (set_up_server(&server) == -1) {
		ERROR("set_up_server", -1);
	}

	while (1) {
	}

	return 0;
}
