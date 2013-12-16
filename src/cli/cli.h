#ifndef _CLI_H
#define _CLI_H 1

/** @file
 * Main program used for the command-line interface
 **/

/** format of the help text */
#define CLI_USAGE_FMT "RTFM"
#define DZCLI_TYPE_SEPARATOR ","

int add(char *file, int in);
int mk_tlv(int argc, char **argv, int in, int out);
int dump_tlv(int argc, char **argv, int out);
int dump_dz(int argc, char **argv, int out);
int compact_dz(char *file);
#endif
