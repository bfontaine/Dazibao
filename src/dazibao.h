typedef char value_t;

struct tlv {
	unsigned byte type;
	unsigned int length:24;
	value_t *value;
};

struct dazibao {
	int fd;
};

/*  */
int open_dazibao(struct dazibao* d, char* path, int flag);

/*  */
int close_dazibao(struct dazibao* d);

/*  */
int read_tlv(struct dazibao* d, struct tlv* buf, int offset);

/*  */
int next_tlv(struct dazibao* d, struct tlv*);

/*  */
int tlv_at(struct dazibao* d, struct tlv*, int offset);

/*  */
int add_tlv(struct dazibao* d, struct tlv*);

/*  */
int rm_tlv(struct dazibao* d, int offset);

/*  */
int compact_dazibao(struct dazibao* d);
