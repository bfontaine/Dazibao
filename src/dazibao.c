#include "dazibao.h"

int open_dazibao(struct dazibao* d, char* path, int flag) {
	return 0;
}

int close_dazibao(struct dazibao* d) {
	return 0;
}

int read_tlv(struct dazibao* d, struct tlv* buf, int offset) {
	return 0;
}

int next_tlv(struct dazibao* d, struct tlv* buf) {
	return 0;
}

int tlv_at(struct dazibao* d, struct tlv*, int offset) {
	return 0;
}

int add_tlv(struct dazibao* d, struct tlv*) {
	return 0;
}

int rm_tlv(struct dazibao* d, int offset) {
	return 0;
}

int compact_dazibao(struct dazibao* d) {
	return 0;
}
