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
#include "logging.h"
#include "notifutils.h"
#include "notification-client.h"

static char notifier_enabled = 1;
static char *notifier = NULL;
static char cmd[NC_BUFFLEN*2];

int check_notifier(void) {
        if (notifier == NULL) {
                LOGINFO("No notifier enabled.");
                return 0;
        } else if (notify("Welcome!", "dazibao-client") == 0) {
                LOGINFO("Notifier enabled.");
                return 1;
        } else {
                LOGERROR("Failed using %s. Disabled notifier.", notifier);
                return 0;
        }
}

void print_notification(char *title, char *msg) {
        printf("\t%s\n\t\t%s\n", title, msg);
        fflush(stdout);
}

int notify(char *title, char *msg) {

        int status = 0;

        if (notifier_enabled) {
                snprintf(cmd, NC_BUFFLEN*2-1, notifier, title, msg);
                status = system(cmd) == -1 ? -1 : 0;
        }

        print_notification(title, msg);
        return status;
}

int read_notifications(char *buf, int len) {

        while (1) {
                char *msg;
                char *p = memchr(buf, '\n', len);
                if (p == NULL) {
                        if (len >= NC_BUFFLEN) {
                                fprintf(stderr, "Notification too long!\n");
                                return -1;
                        }
                        return len;
                }

                switch (buf[0]) {

                case 'C':
                        msg = calloc(sizeof(*msg), p - buf);
                        strncpy(msg, buf + 1, p - buf - 1);
                        notify("Dazibao changed", msg);
                        free(msg);
                        break;
                case 'E':
                        msg = calloc(sizeof(*msg), p - buf);
                        strncpy(msg, buf + 1, p - buf - 1);
                        notify("Error", msg);
                        free(msg);
                        break;
                default:
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

        char buf[NC_BUFFLEN];
        int bufptr;

        bufptr = 0;

        while (1) {
                int rc = read(fd, buf + bufptr, NC_BUFFLEN - bufptr);

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
        char *s_path = "";


        _log_level = LOG_LVL_DEBUG;
        memset(&sun, 0, sizeof(sun));
        sun.sun_family = AF_UNIX;

        struct s_option opt[] = {
                {"--path", ARG_TYPE_STRING, (void *)&(s_path)},
                {"--notifier", ARG_TYPE_STRING, (void *)&(notifier)}
        };

        struct s_args args = {NULL, NULL, opt};

        if (jparse_args(argc - 1, &argv[1], &args,
                                sizeof(opt)/sizeof(*opt)) != 0) {
                ERROR("parse_args", -1);
        }

        if (strcmp(s_path, "") == 0) {
                strncpy(sun.sun_path, getenv("HOME"), UNIX_PATH_MAX - 1);
                strncat(sun.sun_path, "/", UNIX_PATH_MAX - 1);
                strncat(sun.sun_path, ".dazibao-notification-socket",
                        UNIX_PATH_MAX - 1);
        } else {
                strncpy(sun.sun_path, s_path, UNIX_PATH_MAX - 1);
        }

        notifier_enabled = check_notifier();

        fd = socket(PF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
                PERROR("socket");
                exit(1);
        }

        if (connect(fd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
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
