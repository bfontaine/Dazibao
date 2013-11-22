#include "notification-client.h"

static char notifier_enabled = 0;

static char notifier[] = "/usr/bin/notify-send";

int check_notifier(void) {

	printf("[pid:%d] Looking for %s\n", getpid(), notifier);

	if (access(notifier, X_OK) == -1) {
		ERROR("access", 0);
	}
	printf("[pid:%d] %s found\n", getpid(), notifier);
	return 1;
}

void print_notification(char *title, char *msg) {
	printf("\t%s\n\t\t%s\n", title, msg);
	fflush(stdout);
}

void notify(char *title, char *msg) {

	if (notifier_enabled) {

		int pid;
		pid = fork();

		if (pid == -1) {
			PERROR("fork");
			return;
		}
		if (pid == 0) {
			execlp(notifier, notifier, title, msg, NULL);
		} else {
			if (waitpid(pid, NULL, 0) == -1) {
				PERROR("wait");
			}
		}
	}

	print_notification(title, msg);

}


int main(int argc, char **argv) {
	notifier_enabled = check_notifier();
	char buf[BUFFER_SIZE];
	struct sockaddr_un sun;
	int fd, bufptr, rc;

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;

	if (argc == 1) {
		strncpy(sun.sun_path, getenv("HOME"), 107);
		strncat(sun.sun_path, "/", 107);
		strncat(sun.sun_path, ".dazibao-notification-socket", 107);
	} else {
		strncpy(sun.sun_path, argv[1], 107);
	}

	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(fd < 0) {
		perror("socket");
		exit(1);
	}

	rc = connect(fd, (struct sockaddr*)&sun, sizeof(sun));
	if(rc < 0) {
		perror("connect");
		exit(1);
	}

	printf("[pid:%d] Connected to %s\n", getpid(), sun.sun_path);

	bufptr = 0;
	while(1) {
		char *p;
		rc = read(fd, buf + bufptr, BUFFER_SIZE - bufptr);
		if(rc < 0) {
			if(errno == EINTR)
				continue;
			perror("read");
			exit(1);
		}

		if(rc == 0)
			break;

		bufptr += rc;

		p = memchr(buf, '\n', bufptr);
		if(p == NULL) {
			if(bufptr >= BUFFER_SIZE) {
				fprintf(stderr, "Notification too long!\n");
				exit(1);
			}
			continue;
		}

		if(buf[0] == 'C') {
			char *msg;
			msg = calloc(sizeof(*msg), p - buf);
			strncpy(msg, buf + 1, p - buf - 1);
			notify("Dazibao changed", msg);
			free(msg);
		} else {
			fprintf(stderr, "Unknown notification type %c.\n", buf[0]);
		}

		if(p + 1 >= buf + bufptr) {
			bufptr = 0;
		} else {
			memmove(buf, p + 1, buf + bufptr - (p + 1));
			bufptr -= p + 1 - buf;
		}
	}

	return 0;
}
