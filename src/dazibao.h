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

#define ERROR(str, i) {				\
		perror((str));			\
		return (i);			\
	}

#define CLOSE_AND_ERROR(fd, msg, i) {		\
		if(close((fd)) == -1) {		\
			PANIC("close:");	\
		}				\
		ERROR((msg), (i));		\
	}

#define DAZIBAO_HEADER_SIZE 4
#define TLV_HEADER_SIZE 4
#define TYPE_SIZE 1
#define EOD 0;
#define MAGIC_NUMBER 53

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
 * open $(path) file with $(flags) flags
 * apply flock according to $(flags)
 * check if $(path) is valid dazibao
 * fill $(d)
 * returns 0 on success
 */
int open_dazibao(struct dazibao* d, char* path, int flags);

/*
 * release lock on file held by $(d)
 * close this file
 * returns 0 on success
 */
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
