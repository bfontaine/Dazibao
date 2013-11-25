#include "dazibao.h"

#define BUFFLEN 128

int dz_create(dz_t *daz_buf, char *path) {

	int fd;
	char header[DAZIBAO_HEADER_SIZE];

	fd = open(path, O_CREAT | O_EXCL, 0644);

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

	*daz_buf = fd;

	return 0;
}

int dz_open(dz_t *d, char *path, int flags) {

	int fd, lock;
	char header[DAZIBAO_HEADER_SIZE];

	fd = open(path, flags);
	if (fd == -1) {
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
		/* FIXME: calling perror makes no sense here... */
		CLOSE_AND_ERROR(fd, "not a dazibao", -1);
	}

	*d = fd;

	return 0;
}

int dz_close(dz_t *d) {

	if (flock(*d, LOCK_UN) == -1) {
                perror("flock");
                return -1;
	}
	if (close(*d) == -1) {
                perror("close");
                return -1;
	}

	return 0;
}

int dz_read_tlv(dz_t *d, tlv_t *tlv, off_t offset) {

	/* FIXME probably some issues to fix with large tlv */

	if (SET_OFFSET(*d, offset + TLV_SIZEOF_HEADER) == -1) {
		ERROR("lseek", -1);
	}


	return tlv_read(tlv, *d);
}

off_t dz_next_tlv(dz_t *d, tlv_t *tlv) {

	int size_read;
	off_t off_init;

	/*
	 * Precondition: *tlv have to be (at least) TLV_SIZEOF_HEADER long
	 */
        if (sizeof(*tlv) < TLV_SIZEOF_HEADER) {
                return -1;
        }

	off_init = GET_OFFSET(*d);
	
	if (off_init == -1) {
		ERROR("lseek", -1);
	}

	/* try to read regular header (TLV_SIZEOF_HEADER) */
	size_read = read(*d, *tlv, TLV_SIZEOF_HEADER);
	if (size_read == 0) {
		/* reached end of file */
		return EOD;
	} else if (size_read < 0) {
		/* read error */
		ERROR(NULL, -1);
	} else if (size_read < TLV_SIZEOF_TYPE) {
		ERROR(NULL, -1);
	} else if (tlv_get_type(*tlv) == TLV_PAD1) {
		/* we read too far, because TLV_PAD1 is only 1 byte sized */
		if (SET_OFFSET(*d, (off_init + TLV_SIZEOF_TYPE)) == -1) {
			ERROR(NULL, -1);
		}
	} else if (size_read < TLV_SIZEOF_HEADER) {
		/* TODO: loop waiting for read effectively read a whole tlv */
	} else {
		if (SET_OFFSET(*d, (off_init + TLV_SIZEOF(*tlv))) == -1) {
			ERROR(NULL, -1);
		}
	}

	return off_init;
}

int dz_tlv_at(dz_t *d, tlv_t *tlv, off_t offset) {

        /* SAVE_OFFSET(*d); */

	if (SET_OFFSET(*d, offset) == -1) {
		ERROR("lseek", -1);
	}

	if (dz_next_tlv(d, tlv) <= 0) {
                /* RESTORE_OFFSET(*d); */
		return -1;
	}

        /* RESTORE_OFFSET(*d); */
	return 0;
}

int dz_write_tlv_at(dz_t *d, tlv_t tlv, off_t offset) {

        /* SAVE_OFFSET(*d); */

	if (SET_OFFSET(*d, offset) == -1) {
		ERROR("lseek", -1);
	}

	if (tlv_write(tlv, *d) == -1) {
		ERROR("tlv_write", -1);
	}

        /* RESTORE_OFFSET(*d); */
	return 0;
}


int dz_add_tlv(dz_t *d, tlv_t tlv) {
	off_t pad_off, eof_off;

        /* SAVE_OFFSET(*d); */

	/* find EOF offset */
	eof_off = lseek(*d, 0, SEEK_END);

	if (eof_off == 0) {
		ERROR(NULL, -1);
	}

	/* find offset of pad serie leading to EOF */
	pad_off = dz_pad_serie_start(d, eof_off);

	/* set new position if needed (already is EOF offset) */
	if (pad_off != eof_off && SET_OFFSET(*d, pad_off) == -1) {
		ERROR("SET_OFFSET", -1);
	}

	/* write */
	if (dz_write_tlv_at(d, tlv, pad_off) < 0) {
		ERROR(NULL, -1);
	}

	/* truncate if needed */
        if (eof_off > pad_off + (int)TLV_SIZEOF(tlv)
		&& ftruncate(*d, (pad_off + TLV_SIZEOF(tlv))) < 0 ) {
                ERROR("ftruncate", -1);
        }

        /* RESTORE_OFFSET(*d); */

	return 0;
}

off_t dz_pad_serie_start(dz_t *d, off_t offset) {
	off_t off_start, off_tmp;

        tlv_t buf = (tlv_t)malloc(sizeof(char)*TLV_SIZEOF_HEADER);

        if (buf == NULL) {
                ERROR("malloc", -1);
        }

        /* SAVE_OFFSET(*d); */

	if (SET_OFFSET(*d, DAZIBAO_HEADER_SIZE) == -1) {
                free(buf);
		ERROR("lseek", -1);
	}

	off_start = -1;
	
	while ((off_tmp = dz_next_tlv(d, &buf)) != -1
		&& off_tmp != EOD
		&& off_tmp < offset) {
		if (tlv_get_type(buf) == TLV_PAD1
			|| tlv_get_type(buf) == TLV_PADN) {
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
                free(buf);
		ERROR("", -1);
	}

	if(off_start == -1) {
		off_start = offset;
	}

        /* RESTORE_OFFSET(*d); */

        free(buf);
	return off_start;

}

off_t dz_pad_serie_end(dz_t *d, off_t offset) {
	off_t off_stop;
        tlv_t buf = (tlv_t)malloc(sizeof(char)*TLV_SIZEOF_HEADER);

        /* SAVE_OFFSET(*d); */

	if(SET_OFFSET(*d, offset) == -1) {
                free(buf);
		ERROR("lseek", -1);
	}

	/* skip current tlv */
	off_stop = dz_next_tlv(d, &buf);

	/* look for the first tlv which is not a pad */
	while (off_stop != EOD
		&& (off_stop = dz_next_tlv(d, &buf)) > 0) {
		if (tlv_get_type(buf) != TLV_PAD1
                                && tlv_get_type(buf) != TLV_PADN) {
			/* tlv found */
			break;
		}
	}

	if (off_stop == EOD) {
		off_stop = lseek(*d, 0, SEEK_END);
		if (off_stop < 0) {
                        free(buf);
			ERROR("lseek", -1);
		}
	}

        /* RESTORE_OFFSET(*d); */
        free(buf);
	return off_stop;
}


int dz_rm_tlv(dz_t *d, off_t offset) {

	off_t off_start, off_end, off_eof;
        int status;

        /* SAVE_OFFSET(*d); */

	off_start = dz_pad_serie_start(d, offset);
	off_end   = dz_pad_serie_end(d, offset);

	off_eof = lseek(*d, 0, SEEK_END);

	if (off_eof == -1) {
                /* RESTORE_OFFSET(*d); */
		ERROR(NULL, -1);
	}

	if (off_end == off_eof) { /* end of file reached */
		if (ftruncate(*d, off_start) == -1) {
                        perror("ftruncate");
                }
                /* RESTORE_OFFSET(*d); */
		return 0;
	}

        status = dz_do_empty(d, off_start, off_end - off_start);
        /* RESTORE_OFFSET(*d); */
        return status;
}

int dz_do_empty(dz_t *d, off_t start, off_t length) {

        tlv_t buff = (tlv_t)calloc(length, sizeof(char));
        int status = 0;
        char pad1s[TLV_SIZEOF_HEADER-1];

        if (buff == NULL) {
                ERROR("calloc", -1);
        }

        /* SAVE_OFFSET(*d); */

        if (d == NULL || start < DAZIBAO_HEADER_SIZE || length < 0) {
                status = -1;
                goto OUT;
        }

        if (length == 0) {
                goto OUT;
        }

        if (SET_OFFSET(*d, start) == -1) {
                PERROR("lseek");
                status = -1;
                goto OUT;
        }

    	while (length > (TLV_SIZEOF_HEADER+2)) {
                int tmp = length;
                if (length > TLV_MAX_SIZE) {
                    length = TLV_MAX_SIZE;
                }
	    	/* set type */
                tlv_set_type(&buff, TLV_PADN);
    		/* set length */
	    	htod(length - TLV_SIZEOF_HEADER, tlv_get_length_ptr(buff));

	    	if(dz_write_tlv_at(d, buff, start) == 1) {
                        ERROR(NULL, -1);
                }
                start = start + length;
                length = tmp - length;

	    }


        /* We don't have enough room to store a padN, so we fill it with
         * pad1's
         */
        if (length > 0) {
               for (int i=0; i<length; i++) {
                       pad1s[i] = TLV_PAD1;
               }
               if (write(*d, pad1s, length) < 0) {
                       PERROR("write");
               }
        }

        /* RESTORE_OFFSET(*d); */

OUT:
        free(buff);
        return status;
}

int dz_compact(dz_t *d) {

        tlv_t tlv = malloc(sizeof(*tlv)*TLV_SIZEOF_HEADER);
        off_t reading = DAZIBAO_HEADER_SIZE,
              writing = -1;

        int status = 0;

        char buff[BUFFLEN];

        if (d == NULL) {
                status = -1;
                goto OUT;
        }


        if (SET_OFFSET(*d, reading) == -1) {
                status = -1;
                goto OUT;
        }

        while ((reading = dz_next_tlv(d, &tlv)) != EOD) {
                int type = tlv_get_type(tlv);

                if ((type == TLV_PAD1) || (type == TLV_PADN)) {
                        if (writing == -1) {
                                writing = reading;
                        }
                } else{
                        if (writing != -1) {
                                if (SET_OFFSET(*d, (reading +
                                    TLV_SIZEOF_HEADER)) == -1) {
                                    PERROR("lseek");
                                    status = -1;
                                    goto OUT;
                                }

                                int len = tlv_get_length(tlv);
                                tlv_t tlv_tmp;

                                tlv_tmp = realloc(tlv,
                                sizeof(*tlv) * (TLV_SIZEOF_HEADER + len));

                                if (tlv_tmp == NULL) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }
                                tlv = tlv_tmp;

                                if (read(*d, tlv_get_value_ptr(tlv),len)
                                        < len) {
                                    PERROR("read");
                                    status = -1;
                                    goto OUT;
                                }

                                if (dz_write_tlv_at(d, tlv, writing) == -1) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }

                                writing = GET_OFFSET(*d);
                                if (writing == -1) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }

                                if (SET_OFFSET(*d, (reading +
                                    TLV_SIZEOF(tlv))) == -1) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }
                        }
                }
        }

        if (writing != -1) {
            ftruncate(*d, writing);
        }

OUT:
        free(tlv);
        return status;
}

int dz_dump_compound(dz_t *daz_buf, off_t end, int depth, int indent) {

	tlv_t tlv = malloc(sizeof(*tlv)*TLV_SIZEOF_HEADER);
        off_t off;
        char *ind;
        if (indent > 0) {
                ind = malloc(sizeof(char)*indent+1);
                int i;
                for(i = 0; i < indent; i++) {
                        ind[i] = '\t';
                }
                ind[indent]='\0';
        } else {
                ind = malloc(sizeof(char)*1);
                ind[0]='\0';
        }
	
        while (((off = dz_next_tlv(daz_buf, &tlv)) != end )
                        && (off != EOD)) {
                printf("%s",ind);
                int type, len;
                type = tlv_get_type(tlv);
                len = type == TLV_PAD1 ? 0 : tlv_get_length(tlv);
		printf("[%4d] TLV %3d | %8d | ",
			(int)off, tlv_get_type(tlv), len);

                if (type == TLV_COMPOUND ) {
                        printf("COMPOUND \n");
                        if (depth > 0) {
                                off_t current = GET_OFFSET(*daz_buf);
                                SET_OFFSET(*daz_buf, off + TLV_SIZEOF_HEADER);
                                if (dz_dump_compound(daz_buf,current,
                                        (depth-1), (indent+1))) {
                                        ERROR(NULL,-1);
                                }
                                SET_OFFSET(*daz_buf, current);
                                continue;
                        }
                } else if (type == TLV_DATED) {
                        printf("DATE\n");
                        if (depth > 0) {
                                /* TODO
                                function to print date
                                */
                                off_t current = GET_OFFSET(*daz_buf);
                                SET_OFFSET(*daz_buf, off + TLV_SIZEOF_HEADER +
                                TLV_SIZEOF_DATE);
                                if (dz_dump_compound(daz_buf,current,
                                                (depth-1), (indent+1))) {
                                        ERROR(NULL,-1);
                                }
                                SET_OFFSET(*daz_buf, current);
                        }
                } else if (type == TLV_PNG) {
                        printf("PNG\n");
                } else if (type == TLV_JPEG) {
                        printf("JPEG\n");
                } else if (type == TLV_TEXT) {
                        printf("TEXTE\n");
                 } else {
                        printf("...\n");
                }

        }
        if (indent < 0 ) {
                free(ind);
        }
	free(tlv);
        return 0;
}

int dz_dump(dz_t *daz_buf) {
        return dz_dump_compound(daz_buf, EOD, 0,0);
}

#undef BUFFLEN
