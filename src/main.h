#ifndef _MAIN_H
#define _MAIN_H 1

/** @file
 * Main program used for the command-line interface
 **/

#endif

int cmd_add(int argc, char ** argv, char * daz);
int action_add(int argc, char **argv, int flag_compound, int flag_dazibao
, int flag_date, char **daz);
int action_no_option_add(char *daz, unsigned char type);
int cmd_rm(int argc, char ** argv, char * daz);
int cmd_dump(int argc, char ** argv, char * daz);
int action_dump(char *daz, int flag_debug, int flag_depth);
int cmd_compact(int argc, char ** argv, char * daz);
int cmd_create(int argc, char ** argv, char * daz);
void print_usage();
