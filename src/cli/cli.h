#ifndef _CLI_H
#define _CLI_H 1

/** @file
 * Main program used for the command-line interface
 **/

/** format of the help text */
#define CLI_USAGE_FMT "RTFM"

#include "tlv.h"

struct cli_in_info {
        int fd;
        size_t size;
        char *src;
};

int add_all(int argc, char **argv);
int cli_add_tlv(char *file, tlv_t *tlv);
int add(char *file, int in);
int mk_tlv(int argc, char **argv, int in, int out);
int dump_tlv(int argc, char **argv, int out);
int dump_dz(int argc, char **argv, int out);
int compact_dz(char *file);
#endif
