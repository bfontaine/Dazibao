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

int sync_file(dz_t *d){
	if(ftruncate(d->fd, d->len) == -1) {
		PERROR("ftruncate");
		LOGERROR("ftruncate failed");
		return -1;
	}

	if (msync(d->data, d->len, MS_SYNC) == -1) {
		LOGERROR("msync failed");
		return -1;
	}
	return 0;
}

int dz_mmap_data(dz_t *d, size_t t) {

	size_t page_size, real_size;
	int prot;
	
	page_size = (size_t) sysconf(_SC_PAGESIZE);
	real_size = (size_t) (ceil(((float)t)/((float)page_size)) * page_size);
	

	if(d->fflags == O_RDWR && ftruncate(d->fd, real_size) == -1) {
		PERROR("ftruncate");
		LOGERROR("ftruncate failed");
		return -1;
	}

	prot = PROT_READ | (d->fflags == O_RDWR ? PROT_WRITE : 0);

	d->data = (char *)mmap(NULL, real_size, prot, MAP_SHARED, d->fd, 0);

	if (d->data == MAP_FAILED) {
		LOGERROR("mmap failed");
		return 1;
	}

	d->space = real_size;
	return 0;
}

int dz_remap(dz_t *d, size_t t) {

	if (d->fflags == O_RDONLY) {
		LOGERROR("Bad permission");
		return -1;
	}
	
	if (sync_file(d) == -1) {
		LOGERROR("sync_file failed");
		return -1;
	}
	
	if (munmap(d->data, d->len) == -1) {
		LOGERROR("munmap failed");
		return -1;
	}

	if (dz_mmap_data(d, t) == -1) {
		LOGERROR("dz_mmap_data");
		return -1;
	}

	return 0;
}

int dz_create(dz_t *d, char *path) {

	char header[DAZIBAO_HEADER_SIZE];

	d->fd = open(path, O_CREAT | O_EXCL | O_RDWR, 0644);

	if (d->fd == -1) {
		ERROR("open", -1);
	}

	if (flock(d->fd, LOCK_EX) == -1) {
		PERROR("flock");
		goto PANIC;
	}

	d->fflags = O_RDWR;
	
	if (dz_mmap_data(d, DAZIBAO_HEADER_SIZE)) {
		LOGERROR("dz_mmap_data");
		goto PANIC;
	}

	d->data[0] = MAGIC_NUMBER;
	d->data[1] = 0;
	d->len = DAZIBAO_HEADER_SIZE;

	goto OUT;	
PANIC:
	if (close(d->fd) == -1) {
		PERROR("close");
	}
	return -1;

OUT:
	if (sync_file(d) == -1) {
		LOGERROR("sync_file");
		return -1;
	}
	return 0;
}

int dz_open(dz_t *d, char *path, int flags) {

	int lock;
	char header[DAZIBAO_HEADER_SIZE];
	struct stat st;

	if (d->fd == -1) {
		ERROR("open", -1);
	}

	if (flags == O_RDWR) {
		lock = LOCK_EX;
	} else if (flags == O_RDONLY) {
		lock = LOCK_SH;
	} else {
		LOGERROR("Bad flags: use O_RDWR or O_RDONLY");
		goto PANIC;
	}
	
	d->fd = open(path, O_RDWR);

	if (flock(d->fd, lock) == -1) {
		PERROR("flock");
		goto PANIC;
	}

	if (fstat(d->fd, &st) < 0) {
		PERROR("fstat");
		goto PANIC;
	}

	if (st.st_size < DAZIBAO_HEADER_SIZE) {
		LOGERROR("Size is too small: not a dazibao");
		goto PANIC;
	}

	d->offset = DAZIBAO_HEADER_SIZE;
	d->len = st.st_size;
	d->fflags = flags;

	if (dz_mmap_data(d, d->len) == -1) {
		LOGERROR("dz_mmap_data failed.");
		goto PANIC;
	}

	if (d->data[0] != MAGIC_NUMBER || d->data[1] != 0) {
		LOGERROR("Wrong dazibao header");
		goto PANIC;
	}

	goto OUT;
PANIC:
	if (close(d->fd) == -1) {
		PERROR("close");
	}
	return -1;
OUT:
	return 0;
}

int dz_close(dz_t *d) {

	if (d->fflags == O_RDWR && sync_file(d) == -1) {
		LOGERROR("sync_file");
		return -1;
	}

	if (flock(d->fd, LOCK_UN) == -1) {
                perror("flock");
                return -1;
	}

	if (close(d->fd) == -1) {
		PERROR("close");
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
		LOGERROR("d->len:%d, d->offset:%d", (int)d->len, (int)d->offset);
		return -1;
	} else {
		tlv_set_type(tlv, d->data[d->offset]);
		d->offset += TLV_SIZEOF_TYPE;
	}

	if (d->len < d->offset + TLV_SIZEOF_LENGTH) {
		LOGERROR("d->len:%d, d->offset:%d", (int)d->len, (int)d->offset);
		return -1;
	}
	
	if (tlv_get_type(tlv) == TLV_PAD1) {
		return off_init;
	}

	memcpy(tlv_get_length_ptr(tlv), d->data + d->offset, TLV_SIZEOF_LENGTH);
	
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
	dz_t tmp = {0, d->fflags, d->len - offset, 0, d->space - offset, d->data + offset};
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

	if ((d->len - pad_off) < (unsigned int)TLV_SIZEOF(tlv)) {
		if (dz_remap(d, d->len + (TLV_SIZEOF(tlv) - pad_off)) == -1) {
			LOGERROR("dz_remap failed: not enough room to store TLV");
			return -1;
		}
	}

	d->len = pad_off + TLV_SIZEOF(tlv);

	/* write */
	if (dz_write_tlv_at(d, tlv, pad_off) < 0) {
		ERROR(NULL, -1);
	}

	if (sync_file(d) == -1) {
		LOGDEBUG("syn_file failed");
	}
	return 0;
}

off_t dz_pad_serie_start(dz_t *d, off_t offset) {

	off_t off_start, off_tmp;
        tlv_t buf;

	dz_t tmp = {0, d->fflags, offset, DAZIBAO_HEADER_SIZE, d->space, d->data};

	tlv_init(&buf);

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

	dz_t tmp = {0, d->fflags, d->len - offset, offset, d->space, d->data};

	tlv_init(&buf);

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

	int len, status;
        tlv_t buff;
        char pad1s[TLV_SIZEOF_HEADER-1];
	char *b_val;

	len = MIN(length, TLV_MAX_VALUE_SIZE);
	b_val = calloc(len, sizeof(*b_val));

	if (b_val == NULL) {
		status = -1;
		goto OUT;
	}

	tlv_init(&buff);
	tlv_set_length(&buff, len);
	tlv_mread(&buff, b_val);
        status = 0;


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
			status = -1;
			goto OUT;
                }
                start = start + length;
                length = tmp - length;

	    }


        /* We don't have enough room to store a padN, so we fill it with
         * pad1's
         */
        if (length > 0) {
		memset(d->data + d->offset, TLV_PAD1, length);
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
