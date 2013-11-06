#ifndef _DAZIBAO_H
#define _DAZIBAO_H 1

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "tlv.h"
#include "utils.h"

#define DAZIBAO_HEADER_SIZE 4
#define EOD 0
#define MAGIC_NUMBER 53

struct dazibao {
	int fd;
};

void htod(unsigned int n, char *tlv);

unsigned int dtoh(char *len);

int get_type(char *tlv);

void set_type(char *tlv, char t);

void set_length(char *tlv, unsigned int n);

char *get_length_ptr(char *tlv);

unsigned int get_length(char *tlv);

char *get_value_ptr(char *tlv);

/*
 * if $(path) exist, then return error -1
 * else, create a file $(path)
 * and fill $(daz_buf)
 * return 0 on success
 */
int create_dazibao(struct dazibao *daz_buf,  char *path);

/*
 * open $(path) file with $(flags) flags
 * apply flock according to $(flags)
 * see 'man 2 open' for more details on flags
 * check if $(path) is valid dazibao
 * fill $(d)
 * returns 0 on success
 */
int open_dazibao(struct dazibao* d,  char* path,  int flags);

/*
 * release lock on file held by $(d)
 * close this file
 * returns 0 on success
 */
int close_dazibao(struct dazibao* d);

/*  */
int read_tlv(struct dazibao* d, char *tlv,  off_t offset);

/* 
 * read from $(d)
 * fill $(buf)
 * return offset of read tlv
 * return EOD if EOF reached
 * return -1 on error
 */
off_t next_tlv(struct dazibao* d, char *tlv);

/* 
 * use next_tlv to fill $(buf) using $(offseet)
 * but does not change current offset in $(d)
 * return 0 on success
 * return -1 on error
 */
int tlv_at(struct dazibao* d, char *tlv,  off_t offset);

/*  */
int add_tlv(struct dazibao* d, char *tlv);

/*
 * return offset of the begining of the serie of pad
 * just before $(offset)
 * if there is none, $(offset) is returned
 */
off_t pad_serie_start (struct dazibao* d,  off_t offset);

/*
 * return offset the next tlv after $(offset)
 * which is NOT a pad, skipping tlv at $(offset)
 * if there is none, EOF offset is returned
 */
off_t pad_serie_end(struct dazibao* d,  off_t offset);

int rm_tlv(struct dazibao* d,  off_t offset);

/* Empty a part of a dazibao, starting at 'start', and of length 'length'.
 * The part is filled with padN's and pad1's.
 */
int empty_dazibao(struct dazibao *d, off_t start, off_t length);

/*
 * Compact a Dazibao file. The file must have been opened in read/write mode,
 * and the Dazibao is NOT closed by the function. Also, the dazibao offset is
 * NOT preserved.
 * The function returns the number of bytes saved by the compacting operation,
 * or -1 if an error occured.
 */
int compact_dazibao(struct dazibao*);


/*
 * print tlvs contained in $(daz_buf) on standard output
 */
int dump_dazibao(struct dazibao *daz_buf);

#endif /* _DAZIBAO_H */
