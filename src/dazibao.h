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
#include <string.h>
#include "tlvs.h"
#include "utils.h"


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
int read_tlv(struct dazibao* d, struct tlv* buf, off_t offset);

/* 
 * read from $(d)
 * fill $(buf)
 * return offset of read tlv
 * return 0 if EOF reached
 * return -1 on error
 */
off_t next_tlv(struct dazibao* d, struct tlv* buf);

/* 
 * use next_tlv to fill $(buf) using $(offseet)
 * but does not change current offset in $(d)
 * return 0 on success
 * return -1 on error
 */
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
