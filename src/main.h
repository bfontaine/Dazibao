#ifndef _MAIN_H
#define _MAIN_H 1
#endif

int cmd_add(int argc, char ** argv);
int action_add(char *daz, char *cmd, unsigned char type);
int cmd_rm(int argc, char ** argv);
int cmd_dump(int argc, char ** argv);
int cmd_compact(int argc, char ** argv);
int cmd_create(int argc, char ** argv);
void print_usage();
