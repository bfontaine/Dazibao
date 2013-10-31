#include "dazibao.h"

#define BUFFLEN 128


int create_dazibao(struct dazibao* daz_buf, const char* path) {

	int fd;
	char header[DAZIBAO_HEADER_SIZE];

	if (access(path, F_OK) != -1) {
		printf("creat_dazibao error: file %s already exists\n", path);
		return -1;
	}

	fd = creat(path, 644);

	if (fd == -1) {
		ERROR("open", -1);
	}

	if (flock(fd, LOCK_SH) == -1) {
		CLOSE_AND_ERROR(fd, "flock", -1);
	}

	header[0] = MAGIC_NUMBER;
	header[1] = 0;

	if (write(fd, header, DAZIBAO_HEADER_SIZE) < DAZIBAO_HEADER_SIZE) {
		ERROR("write", -1);
	}

	daz_buf->fd = fd;

	return 0;
}

int open_dazibao(struct dazibao* d, const char* path, const int flags) {

	int fd, lock;
	char header[DAZIBAO_HEADER_SIZE];

	fd = open(path, flags);
	if (fd == -1) { /* open failed */
		ERROR("open", -1);
	}

	if (flags == O_WRONLY) {
		lock = LOCK_EX;
	} else if (flags == O_RDONLY || flags == O_RDWR) {
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
        char tlv_length[TLV_SIZEOF_LENGTH];
	off_t current;

	if ((current = lseek(d->fd, 0, SEEK_CUR)) == -1) {
		ERROR("lseek", -1);
	}

	if ((size_read = read(d->fd, &tlv_type, TLV_SIZEOF_TYPE)) < 0) {
		ERROR("next_tlv read type", -1);
	} else if (!size_read) {
		return EOD;
	}

	buf->type = tlv_type;

	if (tlv_type != TLV_PAD1) {
		size_read = read(d->fd, &tlv_length, TLV_SIZEOF_LENGTH);
	        if (size_read < TLV_SIZEOF_LENGTH) {
			printf("read %d, expected %d\n", size_read,(int)TLV_SIZEOF_LENGTH);
			return -1;
	        }
		if (size_read == -1) {
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

int add_tlv(struct dazibao* d, const struct tlv* src) {
        off_t current;
        off_t previous = -1;

        size_t sizeof_tlv;
        struct tlv buff;
        while ((current = next_tlv(d, &buff)) != EOD){
               if ((buff.type == TLV_PAD1) || (buff.type == TLV_PADN)){
                        if (previous > 0){
                                continue;
                        } else {
                                previous = current;
                        }
                } else {
                        previous = -1;
                }

        }
        
        current = lseek(d->fd,0,SEEK_END); 
        if (previous > 0){
                current = previous;
        }

        sizeof_tlv = TLV_SIZEOF(*src);

        if (src->type == TLV_PAD1){
                sizeof_tlv = TLV_SIZEOF_TYPE; 
        }

        if (write(d->fd, &src, sizeof_tlv) != TLV_SIZEOF(*src)){
                ERROR("add_tlv write tlv",-1);      
        }

        if (ftruncate(d->fd, (current + sizeof_tlv)) < 0 ){
                ERROR("add_tlv ftruncate dazibao",-1);
        }

	return 0;
}

off_t pad_serie_start (struct dazibao* d, const off_t offset) {
	off_t off_start, off_init, off_tmp;
	struct tlv buf;

	/* save current position in dazibao */
	off_init = lseek(d->fd, 0, SEEK_CUR);

	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	if (lseek(d->fd, DAZIBAO_HEADER_SIZE, SEEK_SET) == -1) {
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
	if (lseek(d->fd, off_init, SEEK_SET) == -1) {
		perror("lseek");
	}

	return off_start;

}

off_t pad_serie_end(struct dazibao* d, const off_t offset) {
	off_t off_stop, off_init;
	struct tlv buf;

	/* save current position in dazibao */
	off_init = lseek(d->fd, 0, SEEK_CUR);

	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	if(lseek(d->fd, offset, SEEK_SET) == -1) {
		ERROR("lseek", -1);
	}

	/* skip current tlv */
	off_stop = next_tlv(d, &buf);

	/* look for the first tlv which is not a pad */
	while (off_stop != EOD
		&& (off_stop = next_tlv(d, &buf)) > 0) {
		if (buf.type != TLV_PAD1 && buf.type != TLV_PADN) {
			/* tlv found */
			break;
		}
	}

	if (off_stop == EOD) {
		off_stop = lseek(d->fd, 0, SEEK_END);
		if (off_stop < 0) {
			ERROR("lseek", -1);
		}
	}

	/* restore initial offset */
	if (lseek(d->fd, off_init, SEEK_SET) == -1) {
		perror("lseek");
	}

	printf("off_stop is %d\n", (int)off_stop);

	return off_stop;
}


int rm_tlv(struct dazibao* d, const off_t offset) {

	off_t off_start, off_end;

	off_start = pad_serie_start(d, offset);
	off_end   = pad_serie_end(d, offset);

	if (off_end == EOD) { /* end of file reached */
		ftruncate(d->fd, off_start);
		return 0;
	}

        return empty_dazibao(d, off_start, off_end - off_start);
}

int empty_dazibao(struct dazibao *d, off_t start, off_t length) {
        off_t original = GET_OFFSET(d->fd);

        struct tlv buff;

        int status = 0;

        char pad1s[TLV_SIZEOF_HEADER-1];
        char *zeroes = (char*)calloc(BUFFLEN, sizeof(char));

        if (zeroes == NULL) {
                perror("calloc");
                status = -1;
                goto OUT;
        }

        if (original == -1) {
                perror("lseek");
                status = -1;
                goto OUT;
        }

        if (d == NULL || start < DAZIBAO_HEADER_SIZE || length < 0) {
                status = -1;
                goto OUT;
        }

        if (length == 0) {
                goto OUT;
        }

        if (SET_OFFSET(d->fd, start) == -1) {
                perror("lseek");
                status = -1;
                goto OUT;
        }

        while(length > TLV_SIZEOF_HEADER - 1) {
                off_t tlv_len = MIN(length, TLV_MAX_SIZE);
                int val_len   = tlv_len - TLV_SIZEOF_HEADER;

                buff.type   = TLV_PADN;
                buff.length = val_len;
                if (write(d->fd, &buff, TLV_SIZEOF_HEADER) < 0) {
                        perror("write");
                }

                while (val_len > 0) {
                        int l = MIN(val_len, BUFFLEN);
                        if (write(d->fd, zeroes, l) < 0) {
                                perror("write");
                        }
                        val_len -= l;
                }

                length -= tlv_len;
        }

        /* We don't have enough room to store a padN, so we fill it with
         * pad1's
         */
        if (length > 0) {
               for (int i=0; i<length; i++) {
                       pad1s[i] = TLV_PAD1;
               }
               if (write(d->fd, pad1s, length) < 0) {
                       perror("write");
               }
        }


        if (SET_OFFSET(d->fd, original) == -1) {
                perror("lseek");
                status = -1;
                goto OUT;
        }

OUT:
        free(zeroes);
        return status;
}

int compact_dazibao(struct dazibao* d) {

        struct tlv tlv_buf;
        off_t reading = DAZIBAO_HEADER_SIZE,
              writing = DAZIBAO_HEADER_SIZE;

        int saved = 0,
            readlen;

        char buff[BUFFLEN];

        if (d == NULL) {
                return saved;
        }


        if (lseek(d->fd, reading, SEEK_SET) == -1) {
                return -1;
        }

        while (tlv_at(d, &tlv_buf, reading) > 0) {

                int len = TLV_SIZEOF(tlv_buf);

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

                        if (lseek(d->fd, reading, SEEK_SET) == -1) {
                                return -1;
                        }

                        readlen = read(d->fd, buff, MIN(len, BUFFLEN));
                        if (readlen < 0) {
                                return -1;
                        }

                        if (lseek(d->fd, writing, SEEK_SET) == -1) {
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

}


int dump(struct dazibao *daz_buf) {

	struct tlv tlv_buf;

        off_t off;
        int len;

        while ((off = next_tlv(daz_buf, &tlv_buf)) != EOD) {

                len = tlv_buf.type == TLV_PAD1 ? 0 : tlv_buf.length;

                printf("[%4d] TLV %3d | %8d | ...\n", (int)off, tlv_buf.type, len);

        }

	if (off != EOD) {
		return -1;
	}

        return 0;
}

#undef BUFFLEN
