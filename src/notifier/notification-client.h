#ifndef _NOTIFICATION_CLIENT_H
#define _NOTIFICATION_CLIENT_H 1

/** @file
 * A client for the notifications server
 **/

/** buffer length */
#define NC_BUFFLEN 1024

/* TODO document functions below */

/**
 * ?
 **/
int check_notifier(void);

/**
 * @param title
 * @param msg
 **/
void print_notification(char *title, char *msg);

/**
 * @param title
 * @param msg
 **/
int notify(char *title, char *msg);

/**
 * @param buf
 * @param len
 **/
int read_notifications(char *buf, int len);

/**
 * @param fd
 **/
int receive_notifications(int fd);

#endif
