#include "dazibao.h"
#define BUFFSIZE 512
int main(int argc, char **argv) {

        struct dazibao daz_buf;
	char *daz, *cmd;


        if (argc < 3) {
                printf("Usage:\n\t%s <dazibao> <cmd>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

	daz = argv[1];
	cmd = argv[2];


        if (open_dazibao(&daz_buf, daz, O_RDWR)) {
		exit(EXIT_FAILURE);
	}

	if (!strcmp(cmd, "add")) {
		if (argc < 4) {
			printf("expected type\n");
			exit(EXIT_FAILURE);
		}
		char type = (char)atoi(argv[3]);
		char reader[BUFFSIZE];
		int buff_size = 0;
		char *buff = NULL;
		int read_size;

		while((read_size = read(STDIN_FILENO, reader, BUFFSIZE)) < 0) {
			buff_size += read_size;
			if(buff_size > TLV_MAX_VALUE_SIZE) {
				printf("tlv too large\n");
				exit(EXIT_FAILURE);
			}
			if(!realloc(buff, sizeof(*buff) * buff_size)) {
				perror("realloc");
				exit(EXIT_FAILURE);
			}
			memcpy(buff + (buff_size - read_size), reader, read_size);
		}
		
		struct tlv tlv_buf = {type, buff_size, buff};

		if (add_tlv(&daz_buf, &tlv_buf) == -1) {
			printf("failed adding your tlv\n");
		}

		free(buff);
	} else if (!strcmp(cmd, "rm")) {

	} else if (!strcmp(cmd, "dump")) {

	} else {


	}

	/* FIXME: error handling */
	close_dazibao(&daz_buf);

        exit(EXIT_SUCCESS);
}
