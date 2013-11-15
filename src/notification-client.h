#ifndef _NOTIFICATION_CLIENT_H
#define _NOTIFICATION_CLIENT_H 1

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

#define BUFFER_SIZE 1024

int check_notifier(void);
void print_notification(char *title, char *msg);
void notify(char *title, char *msg);

#endif
