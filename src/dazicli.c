#include "dazibao.h"

#define USAGE "usage: dazicli <file>"
#define BUFFSIZE 512



int next(struct dazibao *daz_buf, struct tlv *tlv_buf) {
	off_t cur_off;
	int len;
	cur_off = next_tlv(daz_buf, tlv_buf);
	if (cur_off == EOD) {
		printf("no more tlv\n");
		return -1;
	}
	
	len = tlv_buf->type == TLV_PAD1 ? 0 : tlv_buf->length;
	printf("[%4d] TLV %3d | %8d | ...\n", (int)cur_off, tlv_buf->type, len);
	return 0;
}

int rm(struct dazibao *daz_buf, off_t off) {

	printf("removing tlv at %d\n", (int)off);
	rm_tlv(daz_buf, off);

	return 0;
}

int add(struct dazibao *daz_buf) {
	
	char input[BUFFSIZE];
	struct tlv tlv_buf;
	
	printf("type:");

	if (fgets(input, BUFFSIZE, stdin) == NULL) {
		perror("fgets");
		return EXIT_FAILURE;
	}

	tlv_buf.type = (char)atoi(input);

	printf("length:");
	
	if (fgets(input, BUFFSIZE, stdin) == NULL) {
		perror("fgets");
		return EXIT_FAILURE;
	}

	tlv_buf.length = atoi(input);
	

	

	tlv_buf.value = malloc(sizeof(*tlv_buf.value) * tlv_buf.length);

	if (!tlv_buf.value) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	add_tlv(daz_buf, &tlv_buf);

	free(tlv_buf.value);

	return 0;
}

void quit() {
	exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {

	struct dazibao daz_buf;
	struct tlv tlv_buf;
	char input[BUFFSIZE];
	int len;
	off_t cur_off, tmp_off;


        if (argc < 2) {
                printf("Usage:\n\t%s <dazibao>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

	if (open_dazibao(&daz_buf, argv[1], O_RDWR) == -1) {
		return EXIT_FAILURE;
	}

	printf("*** Welcome in dazibao manager! ***\n");
	
	while(1) {
		printf("> ");
		if (fgets(input, BUFFSIZE, stdin) == NULL) {
			perror("fgets");
			return EXIT_FAILURE;
		}

		switch (input[0]) {
		case 'n':
		case 'N':
			next(&daz_buf, &tlv_buf);	
			break;
		case 'r':
		case 'R':
			rm(&daz_buf, atoi(&input[1]));
			break;
		case 'a':
		case 'A':
			add(&daz_buf);
			break;
		case 'q':
		case 'Q':
			quit();
			break;
		default:
			printf("%c is not an instruction\n", input[0]);
			break;
		}

	}

	close_dazibao(&daz_buf);

	return EXIT_SUCCESS;
}
