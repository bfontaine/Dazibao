#include "main.h"
#include "dazibao.h"
#include <limits.h>
#include <locale.h>

/** @file */

/** buffer size used in various functions */
#define BUFFSIZE 512

/** main function of the command-line interface */
int main(int argc, char **argv) {

	dz_t daz_buf;
	char *daz, *cmd;

        if (setlocale(LC_ALL, "") == NULL) {
                PERROR("setlocale");
        }

        if (argc < 3) {
                printf("Usage:\n\t%s <dazibao> <cmd>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

	daz = argv[1];
	cmd = argv[2];

	if (!strcmp(cmd, "add")) {
                long tmp_type;
		unsigned char type;
		char reader[BUFFSIZE],
                     *buff = NULL;
		unsigned int buff_size = 0;
		int read_size;

		if (argc < 4) {
			printf("expected type\n");
			exit(EXIT_FAILURE);
		}

		if (dz_open(&daz_buf, daz, O_RDWR)) {
			exit(EXIT_FAILURE);
		}

                tmp_type = strtol(argv[3], NULL, 10);

                if (!IN_RANGE(tmp_type, 1, 256)) {
                        printf("unrecognized type\n");
                        exit(EXIT_FAILURE);
                }

                type = (unsigned char)tmp_type;

		while((read_size = read(STDIN_FILENO, reader, BUFFSIZE)) > 0) {

			buff_size += read_size;

			if(buff_size > TLV_MAX_VALUE_SIZE) {
				printf("tlv too large\n");
				exit(EXIT_FAILURE);
			}

			buff = safe_realloc(buff, sizeof(*buff) * buff_size);

			if(!buff) {
				PERROR("realloc");
				exit(EXIT_FAILURE);
			}

			memcpy(buff + (buff_size - read_size),
                                        reader, read_size);
		}


		tlv_t tlv = malloc((buff_size
                                        + TLV_SIZEOF_HEADER) * sizeof(*tlv));
		tlv_set_type(&tlv, type);
		tlv_set_length(&tlv, buff_size);

		memcpy(tlv_get_value_ptr(tlv), buff, buff_size);

		if (dz_add_tlv(&daz_buf, tlv) == -1) {
			printf("failed adding your tlv\n");
		}

		free(tlv);
		free(buff);
	} else if (!strcmp(cmd, "rm")) {

		if (argc < 4) {
			printf("expected offset\n");
			exit(EXIT_FAILURE);
		}

		if (dz_open(&daz_buf, daz, O_RDWR)) {
			exit(EXIT_FAILURE);
		}

		long off = strtol(argv[3], NULL, 10);

                if (!IN_RANGE(off, 0, LONG_MAX-1)) {
                        printf("wrong offset\n");
                        exit(EXIT_FAILURE);
                }

		if (dz_rm_tlv(&daz_buf, (off_t)off)) {
			printf("rm failed\n");
			dz_close(&daz_buf);
			exit(EXIT_FAILURE);
		}

	} else if (!strcmp(cmd, "dump")) {

		if (dz_open(&daz_buf, daz, O_RDONLY)) {
			exit(EXIT_FAILURE);
		}
                if ( argc < 4 ) {
		        if (dz_dump(&daz_buf)) {
			        printf("dump failed\n");
			        dz_close(&daz_buf);
			        exit(EXIT_FAILURE);
		        }
                } else if (argc > 5) {
			printf("expected type\n");
			exit(EXIT_FAILURE);
                } else {
                        char *cmd_dump, *depth;
                        int dep;
                        cmd_dump = argv[3];
                        depth = argv[4];
                        dep = strtol(depth, NULL, 10);

                        if (!IN_RANGE(dep, 0, 16)) {
                                printf("wrong depth");
                                exit(EXIT_FAILURE);
                        }

                        if ((!strcmp(cmd_dump, "--depth")) && (dep >= 0)) {
                                int dep = strtol(depth, NULL, 10);
                                if (!IN_RANGE(dep, 0, 16)) {
                                        printf("wrong depth");
                                        exit(EXIT_FAILURE);
                                }
                                /* option dump compound with limited depth */
		                if (dz_dump_compound(&daz_buf, EOD, dep,0)) {
			                printf("dump_compound failed\n");
			                dz_close(&daz_buf);
			                exit(EXIT_FAILURE);
		                }

                        } else {
			        printf("expected type\n");
			        exit(EXIT_FAILURE);
                        }
                }

	}  else if (!strcmp(cmd, "create")) {
		
		if (dz_create(&daz_buf, daz)) {
			printf("error during dazibao creation\n");
			exit(EXIT_FAILURE);
		}
		
	} else if (!strcmp(cmd, "compact")) {
		if (dz_open(&daz_buf, daz, O_RDWR)) {
			exit(EXIT_FAILURE);
		}

                if (dz_compact(&daz_buf)) {
			exit(EXIT_FAILURE);
                }

	} else {

        }

	if (dz_close(&daz_buf) < 0) {
                printf("Error while closing the dazibao\n");
                exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
}
