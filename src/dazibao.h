#ifndef DAZIBAO_H
#define DAZIBAO_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdlib.h>


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
int read_tlv(struct dazibao* d, struct tlv* buf, int offset);

/*  */
int next_tlv(struct dazibao* d, struct tlv* buf);

/*  */
int tlv_at(struct dazibao* d, struct tlv* buf, int offset);

/*  */
int add_tlv(struct dazibao* d, struct tlv* buf);

/*  */
int rm_tlv(struct dazibao* d, int offset);

/*  */
int compact_dazibao(struct dazibao* d);
#endif /* DAZIBAO_H */
