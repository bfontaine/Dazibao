#include "dazibao.h"

int open_dazibao(struct dazibao* d, char* path, int flags) {

	int fd, lock;
	fd = open(path, flags);
	if(fd == -1) { /* open failed */
		ERROR("open", -1);
	}

	if(flags & O_WRONLY) {
		lock = LOCK_EX;
	} else if(flags & O_RDONLY) {
		lock = LOCK_SH;
	} else {
		fprintf(stderr, "open_dazibao: bad flags\n");
		exit(EXIT_FAILURE);
	}
	
	if(flock(fd, lock) == -1) {
		if(close(fd) == -1) {
			PANIC("close:");
		}
		ERROR("flock", -1);
	}
	
	d->fd = fd;
	
	return 0;
}

int close_dazibao(struct dazibao* d) {

	if(flock(d->fd, LOCK_UN) == -1) {
		PANIC("flock:");
		/* should it return an error intead ? */
	}
	if(close(d->fd) == -1) {
		PANIC("close:");
		/* should it return an error intead ? */
	}

	return 0;
}

int read_tlv(struct dazibao* d, struct tlv* buf, int offset) {
	return 0;
}

int next_tlv(struct dazibao* d, struct tlv* buf) {
	return 0;
}

int tlv_at(struct dazibao* d, struct tlv* buf, int offset) {
	return 0;
}

int add_tlv(struct dazibao* d, struct tlv* buf) {
	return 0;
}

int rm_tlv(struct dazibao* d, int offset) {
	return 0;
}

int compact_dazibao(struct dazibao* d) {
	return 0;
}
