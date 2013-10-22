#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 1024

int
main()
{
    char buf[BUFFER_SIZE];
    struct sockaddr_un sun;
    int fd, bufptr, rc;

    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, getenv("HOME"), 107);
    strncat(sun.sun_path, "/", 107);
    strncat(sun.sun_path, ".dazibao-notification-socket", 107);

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
            printf("Dazibao changed: ");
            fwrite(buf, 1, p - buf, stdout);
            printf("\n");
            fflush(stdout);
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
