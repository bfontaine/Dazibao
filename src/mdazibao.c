#include "mdazibao.h"

/** @file */

#ifndef BUFFLEN
/** buffer length used by various functions */
#define BUFFLEN 128
#endif

int _log_level = LOG_LVL_DEBUG;

/**
 * Look for the beggining of an unbroken pad1/padN serie leading to `offset`.
 * @return offset of the beginning of this serie on success, or the given
 *         offset if there are no pad1/padNs before it.
 * @see dz_pad_serie_end
 */
static off_t dz_pad_serie_start(dz_t *d, off_t offset);

/**
 * Skip tlv at offset, and look for the end of an unbroken pad1/padN serie
 * starting after the skipped tlv.
 * @return offset of the end of this serie on success, or the offset of the
 *         next tlv if it's not followed by pad1/padNs.
 * @see dz_pad_serie_start
 */
static off_t dz_pad_serie_end(dz_t *d, off_t offset);

int dz_create(dz_t *daz_buf, char *path) {

	int fd;
	char header[DAZIBAO_HEADER_SIZE];

	fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);

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

	daz_buf->data = (char *)mmap(NULL, DAZIBAO_HEADER_SIZE,
				PROT_WRITE, MAP_PRIVATE, fd, 0);

	if (daz_buf->data == MAP_FAILED) {
		ERROR("mmap", -1);
	}

	daz_buf->len = DAZIBAO_HEADER_SIZE;
	daz_buf->offset = DAZIBAO_HEADER_SIZE;
	daz_buf->fd = fd;
	
	return 0;
}

int dz_open(dz_t *d, char *path, int flags) {


	/** TODO
	 * clean before exiting on failure
	 */

	int fd, lock;
	char header[DAZIBAO_HEADER_SIZE];

	fd = open(path, flags);
	if (fd == -1) {
		ERROR("open", -1);
	}

	if (flags == O_WRONLY || flags == O_RDWR) {
		lock = LOCK_EX;
	} else if (flags == O_RDONLY) {
		lock = LOCK_SH;
	} else {
		CLOSE_AND_ERROR(fd, "bad flags", -1);
	}

	if (flock(fd, lock) == -1) {
		CLOSE_AND_ERROR(fd, "flock", -1);
	}

	struct stat st;

	if (fstat(fd, &st) < 0) {
		ERROR("fstat", -1);
	}

	if (st.st_size < DAZIBAO_HEADER_SIZE) {
		CLOSE_AND_ERROR(fd, "not a dazibao", -1);
	}


	d->offset = DAZIBAO_HEADER_SIZE;
	d->len = st.st_size;
	d->data = (char *)mmap(NULL, st.st_size, PROT_READ |
			(lock == LOCK_EX ? PROT_WRITE : 0),
			MAP_PRIVATE, fd, 0);
	
	if (d->data == MAP_FAILED) {
		ERROR("mmap", -1);
	}

	if (d->data[0] != MAGIC_NUMBER || d->data[1] != 0) {
		LOGERROR("Wrong dazibao header");
                if (close(fd) == -1) {
                        perror("close");
                }
                return -1;
	}

	return 0;
}

int dz_close(dz_t *d) {

	/** TODO
	 * - Flush buffers
	 * - Free ressources
	 */

	if (flock(d->fd, LOCK_UN) == -1) {
                perror("flock");
                return -1;
	}

	return 0;
}

int dz_read_tlv(dz_t *d, tlv_t *tlv, off_t offset) {
	return tlv_mread(tlv, d->data);
}

off_t dz_next_tlv(dz_t *d, tlv_t *tlv) {
	int rc;
	int off_init = d->offset;
	
	if (d->len == d->offset) {
		return EOD;
	} else if (d->len < d->offset + TLV_SIZEOF_TYPE) {
		LOGERROR("Corrupted Dazibao");
		return -1;
	} else {
		tlv_set_type(tlv, d->data[d->offset]);
		d->offset += TLV_SIZEOF_TYPE;
	}

	if (d->len < d->offset + TLV_SIZEOF_LENGTH) {
		LOGERROR("Corrupted Dazibao");
		return -1;
	}
	
	if (tlv_get_type(tlv) == TLV_PAD1) {
		return off_init;
	}

	memcpy(tlv_get_length_ptr(tlv), d->data  + d->offset, TLV_SIZEOF_LENGTH);
	
	d->offset += TLV_SIZEOF_LENGTH;

	rc = tlv_mread(tlv, d->data + d->offset);

	if (rc < 0) {
		LOGERROR("Failed reading tlv at offset %d", (int)d->offset);
		return -1;
	}

	d->offset += rc;
	return off_init;
}

int dz_tlv_at(dz_t *d, tlv_t *tlv, off_t offset) {
	dz_t tmp = {0, d->len - offset, 0, d->data + offset};
	return dz_next_tlv(&tmp, tlv);
}

int dz_write_tlv_at(dz_t *d, tlv_t *tlv, off_t offset) {
	return tlv_mwrite(tlv, d->data + offset);
}


int dz_add_tlv(dz_t *d, tlv_t *tlv) {
	off_t pad_off, eof_off;

	eof_off = d->len;

	/* find offset of pad serie leading to EOF */
	pad_off = dz_pad_serie_start(d, eof_off);

	/* write */
	if (dz_write_tlv_at(d, tlv, pad_off) < 0) {
		ERROR(NULL, -1);
	}

        if (eof_off > pad_off + (int)TLV_SIZEOF(tlv)) {
		d->len = pad_off + TLV_SIZEOF(tlv);
	}

	return 0;
}

off_t dz_pad_serie_start(dz_t *d, off_t offset) {
	off_t off_start, off_tmp;

	
        tlv_t buf;

	tlv_init(&buf);

	dz_t tmp = {0, d->len - offset, DAZIBAO_HEADER_SIZE, d->data};

	off_start = -1;
	
	while ((off_tmp = dz_next_tlv(&tmp, &buf)) != -1
		&& off_tmp != EOD
		&& off_tmp < offset) {
		if (tlv_get_type(&buf) == TLV_PAD1
			|| tlv_get_type(&buf) == TLV_PADN) {
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

	tlv_destroy(&buf);
	return off_start;

}

off_t dz_pad_serie_end(dz_t *d, off_t offset) {
	off_t off_stop;
        tlv_t buf;
	tlv_init(&buf);
		
	dz_t tmp = {0, d->len - offset, offset, d->data};

	/* skip current tlv */
	off_stop = dz_next_tlv(&tmp, &buf);

	/* look for the first tlv which is not a pad */
	while (off_stop != EOD
		&& (off_stop = dz_next_tlv(&tmp, &buf)) > 0) {
		if (tlv_get_type(&buf) != TLV_PAD1
                                && tlv_get_type(&buf) != TLV_PADN) {
			/* tlv found */
			break;
		}
	}

	if (off_stop == EOD) {
		off_stop = d->len;
	}

	tlv_destroy(&buf);
	return off_stop;
}


int dz_rm_tlv(dz_t *d, off_t offset) {

	off_t off_start, off_end, off_eof;
        int status;

	off_start = dz_pad_serie_start(d, offset);
	off_end   = dz_pad_serie_end(d, offset);

	off_eof = d->len;

	if (off_end == off_eof) { /* end of file reached */
		if (ftruncate(d->fd, off_start) == -1) {
                        PERROR("ftruncate");
                } else {
			d->len = off_start;
		}
		return 0;
	}

        status = dz_do_empty(d, off_start, off_end - off_start);

        return status;
}

int dz_do_empty(dz_t *d, off_t start, off_t length) {

	int len = MIN(length, TLV_MAX_VALUE_SIZE);

	char *b_val = calloc(len, sizeof(*b_val));

        tlv_t buff;
	tlv_init(&buff);
	tlv_set_length(&buff, len);
	tlv_mread(&buff, b_val);
        int status = 0;
        char pad1s[TLV_SIZEOF_HEADER-1];

        if (d == NULL || start < DAZIBAO_HEADER_SIZE || length < 0) {
                status = -1;
                goto OUT;
        }

        if (length == 0) {
                goto OUT;
        }

	d->offset = start;

    	while (length > (TLV_SIZEOF_HEADER + 2)) {
                int tmp = length;
                if (length > TLV_MAX_SIZE) {
                    length = TLV_MAX_SIZE;
                }
	    	/* set type */
                tlv_set_type(&buff, TLV_PADN);
    		/* set length */
                tlv_set_length(&buff, length - TLV_SIZEOF_HEADER);

            	if(dz_write_tlv_at(d, &buff, start) == -1) {
                        ERROR(NULL, -1);
                }
                start = start + length;
                length = tmp - length;

	    }


        /* We don't have enough room to store a padN, so we fill it with
         * pad1's
         */
        if (length > 0) {
		memset(pad1s, TLV_PAD1, length);
		memcpy(d->data + d->offset, pad1s, length);
        }

OUT:
	free(b_val);
        return status;
}

int dz_compact(dz_t *d) {

        tlv_t tlv;
	tlv_init(&tlv);
        off_t reading = DAZIBAO_HEADER_SIZE,
              writing = -1;

        int status = 0;

        char buff[BUFFLEN];

        if (d == NULL) {
                status = -1;
                goto OUT;
        }

	d->offset = reading;

        while ((reading = dz_next_tlv(d, &tlv)) != EOD) {
                int type = tlv_get_type(&tlv);

                if ((type == TLV_PAD1) || (type == TLV_PADN)) {
                        if (writing == -1) {
                                writing = reading;
                        }
                } else {
                        if (writing != -1) {
				d->offset = (reading + TLV_SIZEOF_HEADER);

                                int len = tlv_get_length(&tlv);

				if (tlv_mread(&tlv, d->data + d->offset)) {
					LOGERROR("tlv_mread failed")
					status = -1;
					goto OUT;
				}

                                if (dz_write_tlv_at(d, &tlv, writing) == -1) {
                                    PERROR("realloc");
                                    status = -1;
                                    goto OUT;
                                }
				
                                writing = d->offset;
				d->offset = reading + TLV_SIZEOF(&tlv);
			}
                }
        }

        if (writing != -1) {
            if (ftruncate(d->fd, writing) < 0) {
                    perror("ftruncate");
                    status = -1;
		    goto OUT;
            }
	    d->len = writing;
        }

OUT:
	tlv_destroy(&tlv);
        return status;
}

int dz_dump(dz_t *daz_buf, off_t end, int depth, int indent, int flag_debug) {

	tlv_t tlv;
	if (tlv_init(&tlv) == -1) {
		return -1;
	}

        off_t off;
        char *ind;
        if (indent > 0) {
                ind = malloc(sizeof(*ind) * (indent + 1));
                memset(ind, '\t', indent);
                ind[indent] = '\0';
        } else {
                ind = "";
        }

        while (((off = dz_next_tlv(daz_buf, &tlv)) != end) && (off != EOD)) {

                int tlv_type, len;
                const char *tlv_str;

                printf("%s", ind);

                tlv_type = tlv_get_type(&tlv);
		len = tlv_get_length(&tlv);

                tlv_str = tlv_type2str((char) tlv_type);
                /* for option debug print pad n and pad1 only debug = 1 */
                if (((tlv_type != TLV_PADN) && (tlv_type != TLV_PAD1))
                        || (flag_debug == 1)) {
                        printf("[%9d] TLV %8s | %8d |\n",
                                (int)off, tlv_str, len);
                }

                switch (tlv_type) {
		case TLV_DATED:
			if (depth > 0) {
				off_t current = daz_buf->offset;
				daz_buf->offset = off + TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
				if (dz_dump(daz_buf, current, (depth - 1),
						(indent + 1), flag_debug)) {
					LOGERROR("dz_dump failed");
					return -1;
				}
				daz_buf->offset = current;
			}
			break;
			/* TODO function to print date */
		case TLV_COMPOUND:
			if (depth > 0) {
				off_t current = daz_buf->offset;
				daz_buf->offset = off + TLV_SIZEOF_HEADER;
				if (dz_dump(daz_buf, current, (depth - 1),
						(indent + 1), flag_debug)) {
					LOGERROR("dz_dump failed");
					return -1;
				}
				daz_buf->offset = current;
			}
			break;
		default:
			break;
                }
        }
        if (indent < 0) {
                free(ind);
        }
	tlv_destroy(&tlv);
        return 0;
}

#undef BUFFLEN
