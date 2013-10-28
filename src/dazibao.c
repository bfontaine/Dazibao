#include "dazibao.h"

int open_dazibao(struct dazibao* d, char* path, int flags) {

	int fd, lock;
	char header[DAZIBAO_HEADER_SIZE];

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
	
	if(read(fd, header, DAZIBAO_HEADER_SIZE) < DAZIBAO_HEADER_SIZE) {
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

int read_tlv(struct dazibao* d, struct tlv* buf, off_t offset) {

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

off_t next_tlv(struct dazibao* d, struct tlv* buf) {

	int size_read;
	char tlv_type;
	off_t current;

	if((current = lseek(d->fd, 0, SEEK_CUR)) == -1) {
		ERROR("lseek", -1);
	}

	if((size_read = read(d->fd, &tlv_type, TLV_TYPE_SIZE)) < 0) {
		ERROR("next_tlv read type", -1);
	} else if(!size_read) {
		return EOD;
	}

	buf->type = tlv_type; 
	
	if(tlv_type != TLV_PAD1) {
	        if(read(d->fd, &(buf->length), TLV_LENGTH_SIZE ) < TLV_LENGTH_SIZE) {
		        ERROR("next_tlv read length", -1);
	        }		
                if(lseek(d->fd, buf->length, SEEK_CUR) == -1) {
                        ERROR("next_tlv lseek next_tlv", -1);
                }
        }
	
	return current;
}

int tlv_at(struct dazibao* d, struct tlv* buf, off_t offset) {

	off_t current;
	
	if((current = lseek(d->fd, 0, SEEK_CUR)) == -1) {
		ERROR("lseek", -1);
	}
	
	if(lseek(d->fd, offset, SEEK_SET) == -1) {
		ERROR("lseek", -1);
	}

	if(next_tlv(d, buf) <= 0) {

		if(lseek(d->fd, current, SEEK_SET) == -1) {
			ERROR("lseek", -1);
		}
	
		return -1;
	}

	if(lseek(d->fd, current, SEEK_SET) == -1) {
		ERROR("lseek", -1);
	}

	return 0;
}

int add_tlv(struct dazibao* d, struct tlv* buf) {
	return 0;
}

int rm_tlv(struct dazibao* d, off_t offset) {
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
