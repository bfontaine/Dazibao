#include "notification-client.h"

static char notifier_enabled = 1;
static char *notifier = "/usr/bin/notify-send \"%s\" \"%s\"";
static char cmd[BUFFER_SIZE*2];

int check_notifier(void) {
	printf("[pid:%d] Looking for %s\n", getpid(), notifier);
	return (notify("Welcome!", "dazibao-client") == 0) ? 1 : 0;
}

void print_notification(char *title, char *msg) {
	printf("\t%s\n\t\t%s\n", title, msg);
	fflush(stdout);
}

int notify(char *title, char *msg) {

	int status = 0;

	if (notifier_enabled) {
		sprintf(cmd, notifier, title, msg);
		status = system(cmd) == -1 ? -1 : 0;
	}

	print_notification(title, msg);
	return status;
}


int receive_notifications(int fd) {

	char buf[BUFFER_SIZE];
	int bufptr, rc;
	char *p;

	bufptr = 0;

	while (1) {
		
		rc = read(fd, buf + bufptr, BUFFER_SIZE - bufptr);
		
		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				ERROR("read", -1);
			}
		}

		if (rc == 0) {
			break;
		}
		
		bufptr += rc;

		p = memchr(buf, '\n', bufptr);
		if (p == NULL) {
			if (bufptr >= BUFFER_SIZE) {
				fprintf(stderr, "Notification too long!\n");
				return -1;
			}
			continue;
		}

		if (buf[0] == 'C') {
			char *msg;
			msg = calloc(sizeof(*msg), p - buf);
			strncpy(msg, buf + 1, p - buf - 1);
			notify("Dazibao changed", msg);
			free(msg);
		} else {
			fprintf(stderr, "Unknown notification type %c.\n", buf[0]);
		}

		if (p + 1 >= buf + bufptr) {
			bufptr = 0;
		} else {
			memmove(buf, p + 1, buf + bufptr - (p + 1));
			bufptr -= p + 1 - buf;
		}
	}

	return -1;
}


int main(int argc, char **argv) {

	struct sockaddr_un sun;
	int fd;

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;

	if (argc >= 3) {
		if (strcmp(argv[1], "--path") == 0) {
			strncpy(sun.sun_path, argv[2], 107);
		} else if (strcmp(argv[1], "--notifier") == 0) {
			notifier = argv[2];
		}
		if (argc == 5) {
			if (strcmp(argv[3], "--path") == 0) {
				strncpy(sun.sun_path, argv[4], 107);
			} else if (strcmp(argv[3], "--notifier") == 0) {
				notifier = argv[4];
			}
		}
	}

	if (strcmp(sun.sun_path, "") == 0) {
		strncpy(sun.sun_path, getenv("HOME"), 107);
		strncat(sun.sun_path, "/", 107);
		strncat(sun.sun_path, ".dazibao-notification-socket", 107);
	}

	notifier_enabled = check_notifier();

	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(fd < 0) {
		PERROR("socket");
		exit(1);
	}

	if(connect(fd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
		PERROR("connect");
		exit(1);
	}

	printf("[pid:%d] Connected to %s\n", getpid(), sun.sun_path);
	
	if (receive_notifications(fd) == -1) {
		fprintf(stderr, "[pid:%d] receive_notifications failed. Exiting with 1.\n", getpid());
		exit(1);
	}

	exit(0);
}
