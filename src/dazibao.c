#include "dazibao.h"

int open_dazibao(struct dazibao* d, char* path, int flag) {
	return 0;
}

int close_dazibao(struct dazibao* d) {
	return 0;
}

int read_tlv(struct dazibao* d, struct tlv* buf, offset_t offset) {
	return 0;
}

int next_tlv(struct dazibao* d, struct tlv* buf) {
	return 0;
}

int tlv_at(struct dazibao* d, struct tlv*, offset_t offset) {
	return 0;
}

int add_tlv(struct dazibao* d, struct tlv*) {
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
