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

int dump_tlv(int argc, char **argv, int out) {
        
        dz_t dz;
        tlv_t tlv;
        int status = 0;
        char value = 0;
        int offset = -1;

        struct s_option opt[] = {
                {"--offset", ARG_TYPE_INT, (void *)&offset},
                {"--value", ARG_TYPE_FLAG, (void *)&value},
        };

        struct s_args args = {&argc, &argv, opt};

        if (jparse_args(argc, argv, &args, sizeof(opt)/sizeof(*opt)) != 0) {
                LOGERROR("jparse_args failed");
                return -1;
        }

        if (argv == NULL || offset == -1) {
                LOGERROR("Wrong arguments.");
                return -1;
        }

        if (dz_open(&dz, argv[0], O_RDWR) != 0) {
                LOGERROR("Failed opening %s.", argv[0]);
                return -1;
        }

        if (tlv_init(&tlv) != 0) {
                LOGERROR("Failed initializing TLV.");
                status = -1;
                goto CLOSE;
        }

        switch (dz_tlv_at(&dz, &tlv, offset)) {
        case -1:
/*
  Fixme: dz_tlv_at should always return EOD since it is 0
  case EOD:
 */
                LOGERROR("dz_tlv_at %d failed.", (int)offset);
                status = -1;
                goto DESTROY;
        default:
                break;
        };
        
        dz_read_tlv(&dz, &tlv, offset);
        
        if (value) {
                tlv_fdump_value(&tlv, out);
        } else {
                tlv_fdump(&tlv, out);
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



int dump_dz(int argc, char **argv, int out) {
        
        dz_t dz;
        int status = 0;
        int depth = 0;
        int debug = 0;

        struct s_option opt[] = {
                {"--depth", ARG_TYPE_INT, (void *)&depth},
                {"--debug", ARG_TYPE_FLAG, (void *)&debug},
        };

        struct s_args args = {&argc, &argv, opt};

        if (jparse_args(argc, argv, &args, sizeof(opt)/sizeof(*opt)) != 0) {
                LOGERROR("jparse_args failed");
                return -1;
        }

        if (argv == NULL) {
                LOGERROR("Wrong arguments.");
                return -1;
        }

        if (dz_open(&dz, argv[0], O_RDONLY) != 0) {
                LOGERROR("Failed opening %s.", argv[0]);
                return -1;
        }

        if (dz_dump_all(&dz, depth, debug)) {
                LOGERROR("dz_dump_all failed");
                status = -1;
                goto CLOSE;
        }

CLOSE:
        if (dz_close(&dz) == -1) {
                LOGERROR("Failed closing dazibao.");
                status = -1;
        }

        return status;
}


int compact_dz(char *file) {

        dz_t daz_buf;
        int saved;
        int status = 0;
        if (dz_open(&daz_buf, file, O_RDWR) < 0) {
                return -1;
        }

        saved = dz_compact(&daz_buf);

        if (saved < 0) {
                LOGERROR("dz_compact failed.");
                status = -1;
                goto CLOSE;
        }

CLOSE:
        if (dz_close(&daz_buf) < 0) {
                LOGERROR("dz_close failed");
                status = -1;
                goto OUT;
        }
OUT:
        if (status == 0) {
                LOGINFO("%d bytes saved.\n", saved);
        }
        return status;

}


int main(int argc, char **argv) {

        char *cmd;

        if (setlocale(LC_ALL, "") == NULL) {
                perror("setlocale");
        }

        if (argc < 3) {
                LOGERROR(CLI_USAGE_FMT);
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
        }  else if (strcmp(cmd, "dump_tlv") == 0) {
                if (dump_tlv(argc - 2, &argv[2], STDOUT_FILENO) == -1) {
                        LOGERROR("TLV dumping failed.");
                        return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
        }  else if (strcmp(cmd, "dump") == 0) {
                if (dump_dz(argc - 2, &argv[2], STDOUT_FILENO) == -1) {
                        LOGERROR("Dazibao dumping failed.");
                        return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
        } else if (strcmp(cmd, "compact") == 0) {
                if (compact_dz(argv[2]) == -1) {
                        LOGERROR("Dazibao dumping failed.");
                        return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
        } else {
                return EXIT_FAILURE;
        }
}
