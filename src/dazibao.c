#include "dazibao.h"
#include <wchar.h>
#include <arpa/inet.h>

#define BUFFLEN 128

void htod(long n, char *len) {
	union {
		long i;
		char c[4];
	} tmp;
	tmp.i = htonl(n);
	memcpy(len, &tmp.c[1], 3);
}

long dtoh( char *len) {
	return (len[0] << 16) + (len[1] << 8) + len[2];
}

int get_type(char *tlv) {
	return tlv[0];
}

void set_type(char *tlv, char t) {
	tlv[0] = t;
}

void set_length(char *tlv, long n) {
	htod(n, get_length_ptr(tlv));
}

char *get_length_ptr(char *tlv) {
	return (tlv + TLV_SIZEOF_TYPE);
}

long get_length(char *tlv) {
	return dtoh(get_length_ptr(tlv));
}

char *get_value_ptr(char *tlv) {
	return tlv + TLV_SIZEOF_HEADER;
}

int create_dazibao(struct dazibao* daz_buf,  char* path) {

	int fd;
	char header[DAZIBAO_HEADER_SIZE];

	if (access(path, F_OK) != -1) {
		fprintf(stderr, "create_dazibao error: file %s already exists\n", path);
		return -1;
	}

	fd = creat(path, 0644);

	if (fd == -1) {
		ERROR("creat", -1);
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

int open_dazibao(struct dazibao* d,  char* path,  int flags) {

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

int read_tlv(struct dazibao* d, char *tlv,  off_t offset) {

	/* probably some issues to fix with large tlv */

	/* problem if buf->value was not init with malloc */
	//buf->value = realloc(buf->value, sizeof(*(buf->value)) * buf->length);

	if (SET_OFFSET(d->fd, offset + TLV_SIZEOF_HEADER) == -1) {
		ERROR("lseek", -1);
	}

	int len = dtoh(get_length_ptr(tlv));

	tlv = realloc(tlv, sizeof(*tlv) * (TLV_SIZEOF_HEADER + len));

	if (read(d->fd, get_value_ptr(tlv), len) < len) {
		ERROR("read", -1);
	}

	return 0;
}

off_t next_tlv(struct dazibao* d, char *tlv) {

	/*
	 * PRECONDITION:
	 * tlv have to be (at least) TLV_SIZEOF_HEADER long
	 */

	int size_read;
	off_t off_init;

	off_init = GET_OFFSET(d->fd);
	
	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	/* try to read regular header (TLV_SIZEOF_HEADER) */
	size_read = read(d->fd, tlv, TLV_SIZEOF_HEADER);
	if (size_read == 0) {
		return EOD;
	} else if (size_read < 0) {
		ERROR(NULL, -1);
	} else if (size_read < TLV_SIZEOF_TYPE) {
		ERROR(NULL, -1);
	} else if (get_type(tlv) == TLV_PAD1) {
		/* we read too far, because TLV_PAD1 is only 1 byte sized */
		if (SET_OFFSET(d->fd, (off_init + TLV_SIZEOF_TYPE)) == -1) {
			ERROR(NULL, -1);
		}
	} else if (size_read < TLV_SIZEOF_HEADER) {
		/* TODO: loop waiting for read effectively read all tlv */
	} else {
                // FIXME use TLV_SIZEOF_*
		if (SET_OFFSET(d->fd, (off_init
                                        + TLV_SIZEOF_HEADER
                                        + get_length(tlv))) == -1) {
			ERROR(NULL, -1);
		}
	}
	return off_init;
}

int tlv_at(struct dazibao* d, char *tlv,  off_t offset) {

	off_t off_init;
	off_init = GET_OFFSET(d->fd);
	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	if (SET_OFFSET(d->fd, offset) == -1) {
		ERROR("lseek", -1);
	}

	if (next_tlv(d, tlv) <= 0) {
		if (SET_OFFSET(d->fd, off_init) == -1) {
			ERROR("lseek", -1);
		}
		return -1;
	}

	if (SET_OFFSET(d->fd, off_init) == -1) {
		ERROR("lseek", -1);
	}

	return 0;
}

int write_tlv_at(struct dazibao *d, char *tlv, off_t offset) {

	off_t off_init;
	
	off_init = GET_OFFSET(d->fd);
	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	if (SET_OFFSET(d->fd, offset) == -1) {
		ERROR("lseek", -1);
	}

	if (get_type(tlv) == TLV_PAD1) {
		if (write(d->fd, tlv, TLV_SIZEOF_TYPE) != TLV_SIZEOF_TYPE) {
			ERROR("write", -1);      
		}
		return 0;
	}
	
	int size = TLV_SIZEOF_HEADER + dtoh(get_length_ptr(tlv));

	if (write(d->fd, tlv, size) != size) {
                ERROR("write", -1);      
        }

	if (SET_OFFSET(d->fd, off_init) == -1) {
		ERROR("lseek", -1);
	}
	return 0;
}


int add_tlv(struct dazibao* d,  char *tlv) {
	off_t pad_off, eof_off, off_init;

	/* save current position in dazibao */
	off_init = GET_OFFSET(d->fd);

	if (off_init < 0) {
		ERROR("add_tlv lseek off_init", -1);
	}

	/* find offset of pad serie leading to EOF */

	eof_off = lseek(d->fd, 0, SEEK_END);

	if (eof_off == 0) {
		ERROR(NULL, -1);
	}

	pad_off = pad_serie_start (d, eof_off);

	if (pad_off != eof_off && SET_OFFSET(d->fd, pad_off) == -1) {
		ERROR("SET_OFFSET", -1);
	}

	/* write */
	if (write_tlv_at(d, tlv, pad_off) < 0) {
		ERROR(NULL, -1);
	}

	/* truncate if needed */
        if (eof_off > pad_off + TLV_SIZEOF(tlv)
		&& ftruncate(d->fd, (pad_off + TLV_SIZEOF(tlv))) < 0 ) {
                ERROR("ftruncate", -1);
        }

	/* restore initial offset */
	if (SET_OFFSET(d->fd, off_init) < 0) {
		PERROR("add_tlv lseek restore off_init");
	}

	return 0;
}

off_t pad_serie_start (struct dazibao* d,  off_t offset) {
	off_t off_start, off_init, off_tmp;

	char buf[TLV_SIZEOF_HEADER];

	/* save current position in dazibao */
	off_init = GET_OFFSET(d->fd);

	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	if (SET_OFFSET(d->fd, DAZIBAO_HEADER_SIZE) == -1) {
		ERROR("lseek", -1);
	}

	off_start = -1;
	
	while ((off_tmp = next_tlv(d, buf)) != -1
		&& off_tmp != EOD
		&& off_tmp < offset) {
		if (get_type(buf) == TLV_PAD1
			|| get_type(buf) == TLV_PADN) {
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

off_t pad_serie_end(struct dazibao* d,  off_t offset) {
	off_t off_stop, off_init;
	char buf[TLV_SIZEOF_HEADER];

	/* save current position in dazibao */
	off_init = GET_OFFSET(d->fd);

	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	if(SET_OFFSET(d->fd, offset) == -1) {
		ERROR("lseek", -1);
	}

	/* skip current tlv */
	off_stop = next_tlv(d, buf);

	/* look for the first tlv which is not a pad */
	while (off_stop != EOD
		&& (off_stop = next_tlv(d, buf)) > 0) {
		if (get_type(buf) != TLV_PAD1 && get_type(buf) != TLV_PADN) {
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

	printf("off_stop is %d\n", (int)off_stop);

	return off_stop;
}


int rm_tlv(struct dazibao* d,  off_t offset) {

	/* FIXME: save and restore offset */

	off_t off_start, off_end, off_eof;

	off_start = pad_serie_start(d, offset);
	off_end   = pad_serie_end(d, offset);

	off_eof = lseek(d->fd, 0, SEEK_END);

	if (off_eof == -1) {
		ERROR(NULL, -1);
	}

	if (off_end == off_eof) { /* end of file reached */
		printf("TRUNCATE THE MOTHERFUCKER\n");
		ftruncate(d->fd, off_start);
		return 0;
	}

        return empty_dazibao(d, off_start, off_end - off_start);
}

int empty_dazibao(struct dazibao *d, off_t start, off_t length) {

	/*
	 * FIXME:
	 * DO IT RIGTH
	 */


        off_t original = GET_OFFSET(d->fd);
        char *buff = calloc(length, sizeof(*buff));
        int status = 0;
        char pad1s[TLV_SIZEOF_HEADER-1];

        if (buff == NULL) {
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

	if (length > TLV_SIZEOF_HEADER) {
		/* set type */
		set_type(buff, TLV_PADN);
		/* set length */
		htod(length - TLV_SIZEOF_HEADER, get_length_ptr(buff));

		if(write_tlv_at(d, buff, start) == 1) {
			ERROR(NULL, -1);
		}

	}


        /* We don't have enough room to store a padN, so we fill it with
         * pad1's
         */
        else if (length > 0) {
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
        free(buff);
        return status;
}

/*
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
*/

int dump_dazibao(struct dazibao *daz_buf) {

	char *tlv = malloc(sizeof(*tlv)*TLV_SIZEOF_HEADER);
        off_t off;

#if 0
        while ((off = next_tlv(daz_buf, &tlv_buf)) != EOD) {

                int len = tlv_buf.type == TLV_PAD1 ? 0 : tlv_buf.length;

                if (tlv_buf.type != TLV_TEXT) {
                        printf("[%4d] TLV %3d | %8d | ...\n",
                                        (int)off, tlv_buf.type, len);
                        continue;
                }

                tlv_buf.value = (char*)malloc(sizeof(char)*(tlv_buf.length+1));

                if (tlv_buf.value == NULL) {
                        ERROR("malloc", -1);
                }

                if (read_tlv(daz_buf, &tlv_buf, off) < 0) {
                        ERROR("read_tlv", -1);
                }
                tlv_buf.value[tlv_buf.length] = '\0';

                // These calls to fflush are here to avoid issues with usage
                // of both wprintf and printf
                if (fflush(stdout) == EOF) {
                        perror("fflush");
                }
                wprintf(L"[%4d] TLV %3d | %8d | <%-.10s>\n",
                                (int)off, tlv_buf.type, len,
                                (wchar_t*)tlv_buf.value);
                if (fflush(stdout) == EOF) {
                        perror("fflush");
                }

                /* There may be some possible perf improvements here,
                 * we don't need to free then re-malloc if we read
                 * multiple text TLVs with roughly the same text
                 * length. So we could use malloc once, realloc a few
                 * times if needed, then free.
                 */
                free(tlv_buf.value);
                tlv_buf.value = NULL;
#endif
	
        while ((off = next_tlv(daz_buf, tlv)) != EOD) {
                int len = get_type(tlv) == TLV_PAD1 ? 0 : get_length(tlv);
		printf("[%4d] TLV %3d | %8d | ...\n",
			(int)off, get_type(tlv), len);

        }
	free(tlv);
        return 0;
}

#undef BUFFLEN
