#include "dazibao.h"

int open_dazibao(struct dazibao* d, char* path, int flags) {

	int fd, lock;
	char header[FILE_HEADER_SIZE];

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
		CLOSE_AND_ERROR(fd, "flock", -1);
	}
	
	if(read(fd, header, FILE_HEADER_SIZE) < FILE_HEADER_SIZE) {
		CLOSE_AND_ERROR(fd, "not a dazibao", -1);
	}
	
	if(header[0] != MAGIC_NUMBER || header[1] != 0) {
		/* calling perror makes no sens here... */
		CLOSE_AND_ERROR(fd, "not a dazibao", -1);
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

	/* probably some issues to fix with large tlv */

	/* problem if buf->value was not init with malloc */
	if(!realloc(buf->value, sizeof(*(buf->value)) * buf->length)) {
		ERROR("malloc", -1);
	}

	if(lseek(d->fd, offset, SEEK_SET) == -1) {
		ERROR("lseek", -1);
	}


	if(read(d->fd, buf->value, buf->length) < buf->length) {
		ERROR("read", -1);
	}

	return 0;
}

int next_tlv(struct dazibao* d, struct tlv* buf) {

	/*
	  TODO:
	  - skip PadN tlv
	  - check EOF ?
	  - return offset ?
	*/

	if(read(d->fd, &(buf->type), HEADER_SIZE) < HEADER_SIZE) {
		ERROR("read", -1);
	}
	return 0;
}

int tlv_at(struct dazibao* d, struct tlv* buf, int offset) {

	if(lseek(d->fd, offset, SEEK_SET) == -1) {
		ERROR("lseek", -1);
	}

	return next_tlv(d, buf);
}

int add_tlv(struct dazibao* d, struct tlv* buf) {
	return 0;
}

int rm_tlv(struct dazibao* d, offset_t offset) {
	return 0;
}

int compact_dazibao(struct dazibao* d) {

#define BUFFLEN 128

        struct tlv tlv_buf;
        off_t reading  = DAZIBAO_HEADER_SIZE,
              writing  = DAZIBAO_HEADER_SIZE;

        int saved   = 0,
            readlen,
            len;

        char buff[BUFFLEN];

        if (d == NULL) {
                return saved;
        } 


        if (lseek(d->fd, reading, SEEK_SET) < 0) {
                return -1;
        }

        while(tlv_at(d, &tlv_buf, reading) > 0) {

                len = SIZEOF_TLV(tlv_buf);

                if (tlv_buf.type == PAD1 || tlv_buf.type == PADN) {
                        reading += len;
                        continue;
                }

                if (reading == writing) {
                        reading += len;
                        writing += len;
                        continue;
                }

                saved += len;
                while (len > 0) {

                        if (lseek(d->fd, reading, SEEK_SET) < 0) {
                                return -1;
                        }

                        readlen = read(d->fd, buff, MIN(len, BUFFLEN));
                        if (readlen < 0) {
                                return -1;
                        }

                        if (lseek(d->fd, writing, SEEK_SET) < 0) {
                                return -1;
                        }
                        if (write(d->fd, buff, readlen) < 0) {
                                return -1;
                        }

                        reading += readlen;
                        writing += readlen;
                        len     -= readlen;
                }
        }

        ftruncate(d->fd, reading);

	return saved;

#undef BUFFLEN
}
