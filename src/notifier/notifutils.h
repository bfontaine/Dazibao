#ifndef _NOTIFUTILS_H
#define _NOTIFUTILS_H 1

/** @file
 * Utilities for the notifications server
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum arg_type {
	ARG_TYPE_INT,
	ARG_TYPE_STRING
};

struct s_option {
	char *name;
	enum arg_type type;
	void *value;
};

struct s_args {
	int *argc;
	char ***argv;
	struct s_option *options;
};

/**
 * @param data data to be hashed
 * @param nbytes number of bytes actually hashed (e.g. size of data)
 * @return hashcode
 */
uint32_t qhashmurmur3_32(const void *data, size_t nbytes);

int parse_args(int argc, char **argv, struct s_args *res, int nb_opt);

#endif
