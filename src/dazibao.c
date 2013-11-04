#include "dazibao.h"
#include <wchar.h>

#define BUFFLEN 128


int create_dazibao(struct dazibao* daz_buf, const char* path) {

	int fd;
	char header[DAZIBAO_HEADER_SIZE];

	if (access(path, F_OK) != -1) {
		printf("creat_dazibao error: file %s already exists\n", path);
		return -1;
	}

	fd = creat(path, 0644);

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
	//buf->value = realloc(buf->value, sizeof(*(buf->value)) * buf->length);

	if (!buf->value) {
                return -1;
		//ERROR("realloc", -1);
	}

	if (SET_OFFSET(d->fd, offset + TLV_SIZEOF_HEADER) == -1) {
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

	if ((current = GET_OFFSET(d->fd)) == -1) {
		ERROR("lseek", -1);
	}

	if ((size_read = read(d->fd, &tlv_type, TLV_SIZEOF_TYPE)) < 0) {
		ERROR("next_tlv read type", -1);
	} else if (size_read == 0) {
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

	if ((current = GET_OFFSET(d->fd)) == -1) {
		ERROR("lseek", -1);
	}

	if (SET_OFFSET(d->fd, offset) == -1) {
		ERROR("lseek", -1);
	}

	if (next_tlv(d, buf) <= 0) {

		if (SET_OFFSET(d->fd, current) == -1) {
			ERROR("lseek", -1);
		}
		return -1;
	}

	if (SET_OFFSET(d->fd, current) == -1) {
		ERROR("lseek", -1);
	}

	return 0;
}

int add_tlv(struct dazibao* d, const struct tlv* src) {
        off_t current, off_init;
        off_t previous = -1;
        size_t sizeof_tlv;
        struct tlv buff;

	/* save current position in dazibao */
	off_init = GET_OFFSET(d->fd);

	if (off_init < 0) {
		ERROR("add_tlv lseek off_init", -1);
	}

	if (SET_OFFSET(d->fd, DAZIBAO_HEADER_SIZE) < 0) {
		ERROR("add_tlv lseek dazibao_header", -1);
	}

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
        
        if (previous > 0){
                current = SET_OFFSET(d->fd, previous);
	        if (current < 0) {
		        PERROR("add_tlv lseek previous");
	        }
        } else {
                if ((current = lseek(d->fd,0,SEEK_END)) < 0) {
		        PERROR("add_tlv lseek seek_end");
        	}
        }

        sizeof_tlv = TLV_SIZEOF(*src);

        if (src->type == TLV_PAD1){
                sizeof_tlv = TLV_SIZEOF_TYPE; 
        }

        if (write(d->fd, src, sizeof_tlv) != TLV_SIZEOF(*src)){
                ERROR("add_tlv write tlv",-1);      
        }

        if (ftruncate(d->fd, (current + sizeof_tlv)) < 0 ){
                ERROR("add_tlv ftruncate dazibao",-1);
        }

	/* restore initial offset */
	if (SET_OFFSET(d->fd, off_init) < 0) {
		PERROR("add_tlv lseek restore off_init");
	}

	return 0;
}

off_t pad_serie_start (struct dazibao* d, const off_t offset) {
	off_t off_start, off_init, off_tmp;
	struct tlv buf;

	/* save current position in dazibao */
	off_init = GET_OFFSET(d->fd);

	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	if (SET_OFFSET(d->fd, DAZIBAO_HEADER_SIZE) == -1) {
		ERROR("lseek", -1);
	}

	off_start = -1;
	
	while ((off_tmp = next_tlv(d, &buf)) != -1
		&& off_tmp != EOD
		&& off_tmp < offset) {
		if (buf.type == TLV_PAD1
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

	if (off_tmp == -1) {
		ERROR("", -1);
	}

	if(off_start == -1) {
		off_start = offset;
	}

	/* restore initial offset */
	if (SET_OFFSET(d->fd, off_init) == -1) {
		PERROR("lseek");
	}

	return off_start;

}

off_t pad_serie_end(struct dazibao* d, const off_t offset) {
	off_t off_stop, off_init;
	struct tlv buf;

	/* save current position in dazibao */
	off_init = GET_OFFSET(d->fd);

	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	if(SET_OFFSET(d->fd, offset) == -1) {
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
	if (SET_OFFSET(d->fd, off_init) == -1) {
		PERROR("lseek");
	}

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
                PERROR("calloc");
                status = -1;
                goto OUT;
        }

        if (original == -1) {
                PERROR("lseek");
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
                PERROR("lseek");
                status = -1;
                goto OUT;
        }

        while(length > TLV_SIZEOF_HEADER - 1) {
                off_t tlv_len = MIN(length, TLV_MAX_SIZE);
                int val_len   = tlv_len - TLV_SIZEOF_HEADER;

                buff.type   = TLV_PADN;
                buff.length = val_len;
                if (write(d->fd, &buff, TLV_SIZEOF_HEADER) < 0) {
                        PERROR("write");
                }

                while (val_len > 0) {
                        int l = MIN(val_len, BUFFLEN);
                        if (write(d->fd, zeroes, l) < 0) {
                                PERROR("write");
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
                       PERROR("write");
               }
        }


        if (SET_OFFSET(d->fd, original) == -1) {
                PERROR("lseek");
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


        if (SET_OFFSET(d->fd, reading) == -1) {
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

                        if (SET_OFFSET(d->fd, reading) == -1) {
                                return -1;
                        }

                        readlen = read(d->fd, buff, MIN(len, BUFFLEN));
                        if (readlen < 0) {
                                return -1;
                        }

                        if (SET_OFFSET(d->fd, writing) == -1) {
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


int dump_dazibao(struct dazibao *daz_buf) {

	struct tlv tlv_buf;
        tlv_buf.value = (char*)NULL;
        off_t off;

        while ((off = next_tlv(daz_buf, &tlv_buf)) != EOD) {

                int len = tlv_buf.type == TLV_PAD1 ? 0 : tlv_buf.length;

                if (tlv_buf.type != TLV_TEXT) {
                        printf("[%4d] TLV %3d | %8d | ...\n",
                                        (int)off, tlv_buf.type, len);
                        continue;
                }

                // TODO check for return values
                tlv_buf.value = (char*)malloc(sizeof(char)*(tlv_buf.length+1));
                if (read_tlv(daz_buf, &tlv_buf, off) < 0) {
                        ERROR("read_tlv", -1);
                }
                tlv_buf.value[tlv_buf.length] = '\0';

                wprintf(L"[%4d] TLV %3d | %8d | <%-.10s>\n",
                                (int)off, tlv_buf.type, len,
                                (wchar_t*)tlv_buf.value);
                // TODO check width of wchar_t with gcc

                /* There may be some possible perf improvements here,
                 * we don't need to free then re-malloc if we read
                 * multiple text TLVs with roughly the same text
                 * length. So we could use malloc once, realloc a few
                 * times if needed, then free.
                 */
                free(tlv_buf.value);
                tlv_buf.value = NULL;

        }

        return 0;
}

#undef BUFFLEN
