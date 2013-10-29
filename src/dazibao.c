#include "dazibao.h"

int open_dazibao(struct dazibao* d, const char* path, const int flags) {

	int fd, lock;
	char header[DAZIBAO_HEADER_SIZE];

	fd = open(path, flags);
	if (fd == -1) { /* open failed */
		ERROR("open", -1);
	}

	if (flags & O_WRONLY) {
		lock = LOCK_EX;
	} else if (flags & O_RDONLY) {
		lock = LOCK_SH;
	} else {
		CLOSE_AND_ERROR(fd, "bad flags", -1);
	}

	if (flock(fd, lock) == -1) {
		CLOSE_AND_ERROR(fd, "flock", -1);
	}

	if (read(fd, header, DAZIBAO_HEADER_SIZE) < DAZIBAO_HEADER_SIZE) {
		CLOSE_AND_ERROR(fd, "not a dazibao", -1);
	}

	if (header[0] != MAGIC_NUMBER || header[1] != 0) {
		/* FIXME: calling perror makes no sens here... */
		CLOSE_AND_ERROR(fd, "not a dazibao", -1);
	}

	d->fd = fd;

	return 0;
}

int close_dazibao(struct dazibao* d) {

	if (flock(d->fd, LOCK_UN) == -1) {
		PANIC("flock:");
		/* should it return an error intead ? */
	}
	if (close(d->fd) == -1) {
		PANIC("close:");
		/* should it return an error intead ? */
	}

	return 0;
}

int read_tlv(struct dazibao* d, struct tlv* buf, const off_t offset) {

	/* probably some issues to fix with large tlv */

	/* problem if buf->value was not init with malloc */
	if (!realloc(buf->value, sizeof(*(buf->value)) * buf->length)) {
		ERROR("malloc", -1);
	}

	if (lseek(d->fd, offset, SEEK_SET) == -1) {
		ERROR("lseek", -1);
	}


	if (read(d->fd, buf->value, buf->length) < buf->length) {
		ERROR("read", -1);
	}

	return 0;
}

off_t next_tlv(struct dazibao* d, struct tlv* buf) {

	int size_read;
	char tlv_type;
        char tlv_length[SIZEOF_TLV_LENGTH];
	off_t current;

	if ((current = lseek(d->fd, 0, SEEK_CUR)) == -1) {
		ERROR("lseek", -1);
	}

	if ((size_read = read(d->fd, &tlv_type, SIZEOF_TLV_TYPE)) < 0) {
		ERROR("next_tlv read type", -1);
	} else if (!size_read) {
		return EOD;
	}

	buf->type = tlv_type;

	if (tlv_type != TLV_PAD1) {
	        if (read(d->fd, &tlv_length, SIZEOF_TLV_LENGTH )
                                < SIZEOF_TLV_LENGTH) {
		        ERROR("next_tlv read length", -1);
	        }
                buf->length = (tlv_length[0] << 16) + (tlv_length[1] << 8)
                                + tlv_length[2];
                if (lseek(d->fd, buf->length, SEEK_CUR) == -1) {
                        ERROR("next_tlv lseek next_tlv", -1);
                }
        }

	return current;
}

int tlv_at(struct dazibao* d, struct tlv* buf, const off_t offset) {

	off_t current;

	if ((current = lseek(d->fd, 0, SEEK_CUR)) == -1) {
		ERROR("lseek", -1);
	}

	if (lseek(d->fd, offset, SEEK_SET) == -1) {
		ERROR("lseek", -1);
	}

	if (next_tlv(d, buf) <= 0) {

		if (lseek(d->fd, current, SEEK_SET) == -1) {
			ERROR("lseek", -1);
		}

		return -1;
	}

	if (lseek(d->fd, current, SEEK_SET) == -1) {
		ERROR("lseek", -1);
	}

	return 0;
}

int add_tlv(struct dazibao* d, const struct tlv* buf) {
	return 0;
}

off_t pad_serie_start (struct dazibao* d, const off_t offset) {
	off_t off_start, off_init, off_tmp;
	struct tlv buf;

	/* save current position in dazibao */
	off_init = lseek(d->fd, 0, SEEK_CUR);

	if (off_init < -1) {
		ERROR("lseek", -1);
	}

	if (lseek(d->fd, DAZIBAO_HEADER_SIZE, SEEK_SET) < 0) {
		ERROR("lseek", -1);
	}

	off_start = -1;
	
	while ((off_tmp = next_tlv(d, &buf)) > 0) {
		if(off_tmp == offset) {
			/* tlv to rm reached */
			if (off_start == -1) {
				off_start = offset;
			}
			break;
		} else if (buf.type == TLV_PAD1
			|| buf.type == TLV_PADN) {
			/* pad reached */
			if (off_start == -1) {
				off_start = off_tmp;
			}
		} else {
			/* tlv which is not a pad */
			off_start = -1;
		}
	}

	if (!off_tmp) { /* end of file reached */
		/* FIXME: should not call perror */
		ERROR("no tlv here", -1);
	}

	/* restore initial offset */
	if (lseek(d->fd, off_init, SEEK_SET) < -1) {
		perror("lseek");
	}

	return off_start;

}

off_t pad_serie_end(struct dazibao* d, const off_t offset) {
	off_t off_stop, off_init;
	struct tlv buf;

	/* save current position in dazibao */
	off_init = lseek(d->fd, 0, SEEK_CUR);

	if (off_init < -1) {
		ERROR("lseek", -1);
	}


	if(lseek(d->fd, offset, SEEK_SET) < -1) {
		ERROR("lseek", -1);
	}

	off_stop = offset;

	/* look for the first tlv which is not a pad */
	while ((off_stop = next_tlv(d, &buf)) > 0) {
		if (buf.type != TLV_PAD1 && buf.type != TLV_PADN) {
			/* tlv found */
			break;
		}
	}

	/* restore initial offset */
	if (lseek(d->fd, off_init, SEEK_SET) < -1) {
		perror("lseek");
	}

	return off_stop;
}


int rm_tlv(struct dazibao* d, const off_t offset) {

	off_t off_start, off_end, off_init;
	struct tlv buf;

	/* save current position in dazibao */
	off_init = lseek(d->fd, 0, SEEK_CUR);

	if (off_init < -1) {
		ERROR("lseek", -1);
	}

	off_start = pad_serie_start(d, offset);

	off_end = pad_serie_end(d, offset);

	if (!off_end) { /* end of file reached */
		ftruncate(d->fd, off_start);
		return 0;
	}

	int len = off_end - off_start - SIZEOF_TLV_HEADER;

	/* reach space to erase */
	if (lseek(d->fd, off_start, SEEK_SET) < -1) {
		ERROR("lseek", -1);
	}
	
	if (len < 0) {
		/* not enough space to contain padn, use pad1s */
		/* FIXME: could write in one call */
		int i;
		for(i = 0; i < off_end - off_start; i++) {
			if(write(d->fd, TLV_PAD1, SIZEOF_TLV_TYPE)
				< SIZEOF_TLV_TYPE) {
				ERROR("write", -1);
			}
		}
		goto OUT;
	}
	
	/* writing a padn */
	buf.type = TLV_PADN;
	buf.length = len;
	
	if (write(d->fd, &buf, SIZEOF_TLV_HEADER)) {
                ERROR("write", -1);
        }

OUT:
	if (lseek(d->fd, off_init, SEEK_SET) < -1) {
		perror("lseek");
	}

	return 0;
}

int compact_dazibao(struct dazibao* d) {

/* FIXME: should not define locally */
#define BUFFLEN 128

        struct tlv tlv_buf;
        off_t reading = DAZIBAO_HEADER_SIZE,
              writing = DAZIBAO_HEADER_SIZE;

        int saved = 0,
            readlen;

        char buff[BUFFLEN];

        if (d == NULL) {
                return saved;
        }


        if (lseek(d->fd, reading, SEEK_SET) < 0) {
                return -1;
        }

        while (tlv_at(d, &tlv_buf, reading) > 0) {

                int len = SIZEOF_TLV(tlv_buf);

                if (TLV_IS_EMPTY_PAD(tlv_buf.type)) {
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
