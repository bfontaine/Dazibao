#ifndef _DAZIBAO_H
#define _DAZIBAO_H 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdlib.h>
#include "tlvs.h"
#include "utils.h"


#define PANIC(str) {					\
		perror((str));				\
		exit(EXIT_FAILURE);			\
	}


typedef char value_t;

struct tlv {
	unsigned char type;
	unsigned int length:24;
	value_t *value;
};

struct dazibao {
	int fd;
};

/*
  open $(path) file with $(flag) flag
  apply flock according to $(flag)
  fill $(d)
 */
int open_dazibao(struct dazibao* d, char* path, int flags);

/*  */
int close_dazibao(struct dazibao* d);

/*  */
int read_tlv(struct dazibao* d, struct tlv* buf, off_t offset);

/*  */
int next_tlv(struct dazibao* d, struct tlv* buf);

/*  */
int tlv_at(struct dazibao* d, struct tlv* buf, off_t offset);

/*  */
int add_tlv(struct dazibao* d, struct tlv* buf);

/*  */
int rm_tlv(struct dazibao* d, off_t offset);

/*
 * Compact a Dazibao file. The file must have been opened in read/write mode,
 * and the Dazibao is NOT closed by the function. Also, the dazibao offset is
 * NOT preserved.
 * The function returns the number of bytes saved by the compacting operation,
 * or -1 if an error occured.
 */
int compact_dazibao(struct dazibao*);

#endif /* _DAZIBAO_H */
