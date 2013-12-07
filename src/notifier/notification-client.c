#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "utils.h"
#include "notifutils.h"
#include "notification-client.h"

static char notifier_enabled = 1;
static char *notifier = "/usr/bin/notify-send \"%s\" \"%s\"";
static char cmd[BUFFER_SIZE*2];
int _log_level = LOG_LVL_DEBUG;

int check_notifier(void) {
        LOGINFO("Looking for %s", notifier);
        return (notify("Welcome!", "dazibao-client") == 0) ? 1 : 0;
}

void print_notification(char *title, char *msg) {
        printf("\t%s\n\t\t%s\n", title, msg);
        fflush(stdout);
}

int notify(char *title, char *msg) {

        int status = 0;

        if (notifier_enabled) {
                snprintf(cmd, BUFFER_SIZE*2-1, notifier, title, msg);
                status = system(cmd) == -1 ? -1 : 0;
        }

        print_notification(title, msg);
        return status;
}

int read_notifications(char *buf, int len) {

        while (1) {
                char *p = memchr(buf, '\n', len);
                if (p == NULL) {
                        if (len >= BUFFER_SIZE) {
                                fprintf(stderr, "Notification too long!\n");
                                return -1;
                        }
                        return len;
                }

                if (buf[0] == 'C') {
                        char *msg;
                        msg = calloc(sizeof(*msg), p - buf);
                        strncpy(msg, buf + 1, p - buf - 1);
                        notify("Dazibao changed", msg);
                        free(msg);
                } else {
                        fprintf(stderr, "Unknown notification type: %c.\n",
                                        buf[0]);
                }

                if (p + 1 >= buf + len) {
                        return 0;
                } else {
                        len -= p + 1 - buf;
                        memmove(buf, p + 1, len);
                }
        }
        return len;
}


int receive_notifications(int fd) {

        char buf[BUFFER_SIZE];
        int bufptr;

        bufptr = 0;

        while (1) {
                int rc = read(fd, buf + bufptr, BUFFER_SIZE - bufptr);

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

                bufptr = read_notifications(buf, bufptr + rc);
                if (bufptr == -1) {
                        ERROR("read_notifications", -1);
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
                        strncpy(sun.sun_path, argv[2], UNIX_PATH_MAX - 1);
                } else if (strcmp(argv[1], "--notifier") == 0) {
                        notifier = argv[2];
                }
                if (argc == 5) {
                        if (strcmp(argv[3], "--path") == 0) {
                                strncpy(sun.sun_path, argv[4],
                                                UNIX_PATH_MAX - 1);
                        } else if (strcmp(argv[3], "--notifier") == 0) {
                                notifier = argv[4];
                        }
                }
        }

        if (strcmp(sun.sun_path, "") == 0) {
                strncpy(sun.sun_path, getenv("HOME"), UNIX_PATH_MAX - 1);
                strncat(sun.sun_path, "/", UNIX_PATH_MAX - 1);
                strncat(sun.sun_path, ".dazibao-notification-socket",
                                UNIX_PATH_MAX - 1);
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

        LOGINFO("Connected to %s", sun.sun_path);

        if (receive_notifications(fd) == -1) {
                LOGFATAL("receive_notifications failed. Exiting with 1");
                exit(1);
        }

        exit(0);
}
