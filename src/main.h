#ifndef _MAIN_H
#define _MAIN_H 1
#endif

int cmd_add(int argc, char ** argv, char * daz);
int action_add(char *daz, unsigned char type);
int cmd_rm(int argc, char ** argv, char * daz);
int cmd_dump(int argc, char ** argv, char * daz);
int cmd_compact(int argc, char ** argv, char * daz);
int cmd_create(int argc, char ** argv, char * daz);
void print_usage();
