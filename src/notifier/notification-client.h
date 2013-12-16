#ifndef _NOTIFICATION_CLIENT_H
#define _NOTIFICATION_CLIENT_H 1

/** @file
 * A client for the notifications server
 **/

#define NC_BUFFLEN 1024

int check_notifier(void);
void print_notification(char *title, char *msg);
int notify(char *title, char *msg);
int read_notifications(char *buf, int len);
int receive_notifications(int fd);

#endif
