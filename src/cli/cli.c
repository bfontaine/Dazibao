#include <locale.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <stdint.h>

#include "utils.h"
#include "logging.h"
#include "tlv.h"
#include "cli.h"
#include "mdazibao.h"

int add(char *file, int in) {

        dz_t dz;
        tlv_t tlv;
        int status = 0;

        if (dz_open(&dz, file, O_RDWR) != 0) {
                LOGERROR("Failed opening %s.", file);
                return -1;
        }

        if (tlv_init(&tlv) != 0) {
                LOGERROR("Failed initializing TLV.");
                status = -1;
                goto CLOSE;
        }

        if (tlv_from_file(&tlv, in) == -1) {
                LOGERROR("tlv_from_file failed.");
                status = -1;
                goto CLOSE;
        }

        
        LOGERROR("Inserting TLV type: %d, size:%d",
                tlv_get_type(&tlv), tlv_get_length(&tlv));

        if (dz_add_tlv(&dz, &tlv) != 0) {
                LOGERROR("Failed adding TLV.");
                status = -1;
                goto DESTROY;
        }

DESTROY:
        tlv_destroy(&tlv);

CLOSE:
        if (dz_close(&dz) == -1) {
                LOGERROR("Failed closing dazibao.");
                status = -1;
        }

        return status;
}


int mk_tlv(int argc, char **argv, int in, int out) {

        int date = 0;
        char *type = "";
	struct stat st;
	int status;
	char type_code;
        tlv_t tlv;
        uint32_t timestamp;
	
        struct s_option opt[] = {
                {"--date", ARG_TYPE_FLAG, (void *)&date},
                {"--type", ARG_TYPE_STRING, (void *)&type},
        };

        struct s_args args = {NULL, NULL, opt};

        if (jparse_args(argc, argv, &args, sizeof(opt)/sizeof(*opt)) != 0) {
                LOGERROR("jparse_args failed");
                status = -1;
                goto OUT;
        }

        if (date) {
                timestamp = (uint32_t) time(NULL);
                if (timestamp == (uint32_t) -1) {
                        LOGERROR("Failed getting time.");
                        status = -1;
                        goto OUT;
                }
        } else {
                timestamp = 0;
        }

	type_code = tlv_str2type(type);

	if (type_code == -1) {
                LOGERROR("Undefined type: %s", type);
                status = -1;
                goto OUT;
	}

        if (tlv_init(&tlv) == -1) {
                LOGERROR("tlv_init failed");
                status = -1;
                goto DESTROY;
        }

        if (tlv_file2tlv(&tlv, in, type_code, timestamp)) {
                LOGERROR("tlv_file2tlv failed");
                status = -1;
                goto DESTROY;
        }

        if (tlv_fdump(&tlv, out) == -1) {
                LOGERROR("tlv_fdump failed");
                status = -1;
                goto DESTROY;
        }

DESTROY:
        if (tlv_destroy(&tlv) == -1) {
                LOGERROR("tlv_destroy failed");
                status = -1;
                goto OUT;
        }

OUT:
	return status;	
}


int main(int argc, char **argv) {

        char *cmd;

        if (setlocale(LC_ALL, "") == NULL) {
                perror("setlocale");
        }

        if (argc < 3) {
                printf(CLI_USAGE_FMT, argv[0]);
                exit(EXIT_FAILURE);
        }
        
        cmd = argv[1];

        if (strcmp(cmd, "add") == 0) {
                if (add(argv[2], STDIN_FILENO) != 0) {
                        LOGERROR("TLV addition failed.");
                        return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
        } else if (strcmp(cmd, "mk_tlv") == 0) {
                if (mk_tlv(argc - 2, &argv[2], STDIN_FILENO, STDOUT_FILENO) == -1) {
                        LOGERROR("TLV making failed.");
                        return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
        } else {
                return EXIT_FAILURE;
        }
}
