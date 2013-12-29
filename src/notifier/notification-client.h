#ifndef _NOTIFICATION_CLIENT_H
#define _NOTIFICATION_CLIENT_H 1

/** @file
 * A client for the notifications server
 **/

/** buffer length */
#define NC_BUFFLEN 1024

/**
 * Check if notifier set is available
 * @return 1 on success, 0 on error or if no notifier was set
 **/
int check_notifier(void);

/**
 * Print a notification on stdout
 * @param title title of message
 * @param msg body of message
 **/
void print_notification(char *title, char *msg);

/**
 * Use notifier if enabled, print notification on stdin in any case
 * @param title title of message
 * @param msg body of message
 * @return 0 on success, and -1 if notifier call failed
 **/
int notify(char *title, char *msg);

/**
 * Parse a buffer and process every notification it contains
 * @param buf buffer
 * @param len length of buffer
 * @return number of byte remaining not read in buff, or -1 on error
 **/
int read_notifications(char *buf, int len);

/**
 * Loop reading incoming messages from a socket and using
 * read_notification to process them
 * @param fd socket from where to read
 * @return only return on error (-1)
 **/
int receive_notifications(int fd);

#endif
