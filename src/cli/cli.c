#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "cli.h"
#include "logging.h"
#include "mdazibao.h"
#include "tlv.h"
#include "utils.h"


int cli_mk_tlv(tlv_t *tlv, int argc, char **argv, char *type, char date) {

        int status = 0;
        uint32_t timestamp = 0;
        struct tlv_input *inputs;
        int *fd;
        char *delim = ",";

        if (date) {
                timestamp = (uint32_t) time(NULL);
                timestamp = htonl(timestamp);
                if (timestamp == (uint32_t) -1) {
                        status = -1;
                        goto OUT;
                }
        }

        fd = calloc(argc, sizeof(*fd));

        if (fd == NULL) {
                PERROR("malloc");
                status = -1;
                goto OUT;
        }

        inputs = calloc(argc, sizeof(*inputs));

        if (inputs == NULL) {
                PERROR("malloc");
                status = -1;
                goto CLOSEFD;
        }


        /* prepare inputs information */

        for (int i = 0; i < argc; i++) {

                if (type != NULL) {
                        char *str = i == 0 ? type : NULL;
                        char *tok = strtok(str, delim);
                        inputs[i].type = tlv_str2type(tok);
                        
                        if (inputs[i].type == -1) {
                                LOGERROR("Undefined type.");
                                status = -1;
                                goto CLOSEFD;
                        }
                } else {
                        inputs[i].type = -1;
                }
                
                fd[i] = open(argv[i], O_RDONLY);
                
                if (fd[i] == -1) {
                        if (access(argv[i], F_OK) == 0) {
                                LOGERROR("Failed opening %s", argv[i]);
                                status = -1;
                                goto CLOSEFD;
                        }
                        inputs[i].len = strlen(argv[i]);
                } else {
                        struct stat st;
                        if (fstat(fd[i], &st) == -1) {
                                PERROR("fstat");
                                status = -1;
                                goto CLOSEFD;
                        }
                        if (flock(fd[i], LOCK_SH)) {
                                PERROR("flock");
                                status = -1;
                                goto CLOSEFD;
                        }
                        inputs[i].len = st.st_size;
                        inputs[i].data = mmap(NULL, inputs[i].len,
                                        PROT_READ, MAP_PRIVATE, fd[i], 0);

                        if (inputs[i].data == MAP_FAILED) {
                                PERROR("mmap");
                                status = -1;
                                goto CLOSEFD;
                        }

                }
        }

        if (tlv_from_inputs(tlv, inputs, argc, timestamp) == -1 ) {
                LOGERROR("tlv_from_inputs failed");
                status = -1;
                goto CLOSEFD;
        }

CLOSEFD:
        for (int i = 0; i < argc; i++) {
                if (fd[i] != -1) {
                        if (inputs[i].len != 0
                                && munmap(inputs[i].data,
                                        inputs[i].len) == -1) {
                                PERROR("munmap");
                                status = -1;
                        }
                        if (close(fd[i]) == -1) {
                                PERROR("close");
                                status = -1;
                        }
                }
        }

        free(fd);

        free(inputs);

OUT:
        return status;

}

int cli_add(int argc, char **argv) {

        int status = 0;
        dz_t dz;
        tlv_t tlv;
        char date = 0;
        char *type = NULL;
        char *file;
        char **inputs;
        int nb_inputs;
        /* Parsing arguments. */

        struct s_option opt[] = {
                {"--date", ARG_TYPE_FLAG, (void *)&date},
                {"--type", ARG_TYPE_STRING, (void *)&type},
                {"--dazibao", ARG_TYPE_STRING, (void *)&file}
        };

        struct s_args args = {&nb_inputs, &inputs, opt};

        if (jparse_args(argc, argv, &args, sizeof(opt)/sizeof(*opt)) == -1) {
                LOGERROR("jparse_args failed.");
                status = -1;
                goto OUT;
        }

        LOGINFO("--date:%d, --type:%s, --dazibao:%s", date, type, file);

        if (file == NULL) {
                LOGERROR("Missing arguments (see manual).");
                status = -1;
                goto OUT;
        }


        if (dz_open(&dz, file, O_RDWR) != 0) {
                LOGERROR("Failed opening %s.", file);
                status = -1;
                goto OUT;
        }

        if (tlv_init(&tlv) == -1) {
                LOGERROR("tlv_init failed.");
                status = -1;
                goto CLOSE;
        }

        if (cli_mk_tlv(&tlv, nb_inputs, inputs, type, date) == -1) {
                LOGERROR("cli_mk_tlv failed.");
                status = -1;
                goto DESTROY;
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
OUT:
        return status;
}


int cli_dump_tlv(int argc, char **argv, int out) {

        dz_t dz;
        tlv_t tlv;
        int status = 0;
        char value = 0;
        long long int offset = -1;

        struct s_option opt[] = {
                {"--offset", ARG_TYPE_LLINT, (void *)&offset},
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
                LOGERROR("dz_tlv_at %d failed.", (int)offset);
                status = -1;
                goto DESTROY;
        case EOD:
                LOGINFO("EOD reached.");
                status = -1;
                goto DESTROY;
        default:
                break;
        };

        if (tlv_get_type(&tlv) == TLV_LONGH && value) {
                dz_read_tlv(&dz, &tlv, offset);
                uint32_t len = ltlv_real_data_length(&tlv);
                char *buff = malloc(sizeof(*buff) * len);
                size_t off;
                uint32_t write_idx = 0;

                if (buff == NULL) {
                        PERROR("malloc");
                        status = -1;
                        goto DESTROY;
                }

                if (dz_set_offset(&dz, offset) == -1) {
                        LOGERROR("dz_set_offset failed");
                        status = -1;
                        free(buff);
                }

                dz_next_tlv(&dz, &tlv);

                while ((off = dz_next_tlv(&dz, &tlv)) != EOD) {
                        if (tlv_get_type(&tlv) == TLV_LONGC) {
                                dz_read_tlv(&dz, &tlv, off);
                                tlv_mdump_value(&tlv, buff + write_idx);
                                write_idx += tlv_get_length(&tlv);
                        } else {
                                break;
                        }
                }
                if (write_idx != len) {
                        LOGERROR("Read: %u, expected %u", write_idx, len);
                } else if ((uint32_t)write_all(out, buff, len) != len) {
                        LOGERROR("write_all failed");
                }
                free(buff);
        } else {
                dz_read_tlv(&dz, &tlv, offset);

                if (value) {
                        tlv_fdump_value(&tlv, out);
                } else {
                        tlv_fdump(&tlv, out);
                }
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

int cli_print_long_tlv(dz_t *dz, tlv_t *tlv, int indent, int lvl, int debug) {

        dz_read_tlv(dz, tlv, dz_get_offset(dz));

        int len = ltlv_real_data_length(tlv);
        int type = ltlv_real_data_type(tlv);

        for (int i = 0; i <= indent; i++) {
                printf("--");
        }

        printf(" @[%10li]: %8s (%d bytes)\n",
                dz_get_offset(dz), tlv_type2str(type), len);

        return ltlv_get_total_length(tlv);

}

int cli_print_all_tlv(dz_t *dz, int indent, int lvl, int debug) {

        /**
         * FIXME: This function breaks
         * abstraction of dazibao type
         */

        tlv_t tlv;
        off_t off;

        if (tlv_init(&tlv) == -1) {
                LOGERROR("tlv_init");
                return -1;
        }

        while ((off = dz_next_tlv(dz, &tlv)) != EOD) {

                if (off == -1) {
                        return -1;
                }

                int type = tlv_get_type(&tlv);

                if ((type == TLV_PAD1 || type == TLV_PADN)
                        && !debug) {
                        continue;
                }

                int len = tlv_get_length(&tlv);

                if (type == TLV_LONGH) {
                        dz_set_offset(dz, off);
                        off_t next = cli_print_long_tlv(
                                dz, &tlv, indent, lvl, debug);
                        dz_incr_offset(dz, next);
                        continue;
                }

                for (int i = 0; i <= indent; i++) {
                        printf("--");
                }

                printf(" @[%10li]: %8s (%d bytes)\n",
                        off, tlv_type2str(type), len);


                switch (type) {
                case TLV_DATED:
                        if (lvl != 0) {
                                dz_t cmpnd = {
                                        -1,
                                        0,
                                        off
                                        + TLV_SIZEOF_HEADER
                                        + len,
                                        off
                                        + TLV_SIZEOF_HEADER
                                        + TLV_SIZEOF_DATE,
                                        -1,
                                        dz->data
                                };
                                cli_print_all_tlv(
                                        &cmpnd,
                                        indent + 1,
                                        lvl - 1,
                                        debug);
                        }
                        break;
                case TLV_COMPOUND:
                        if (lvl != 0) {
                                dz_t cmpnd = {
                                        -1,
                                        0,
                                        off
                                        + TLV_SIZEOF_HEADER
                                        + len,
                                        off
                                        + TLV_SIZEOF_HEADER,
                                        -1,
                                        dz->data
                                };
                                cli_print_all_tlv(
                                        &cmpnd,
                                        indent + 1,
                                        lvl - 1,
                                        debug);
                        }
                        break;
                }
        }

        tlv_destroy(&tlv);
        return 0;
}

int cli_dump_dz(int argc, char **argv, int out) {

        dz_t dz;
        int status = 0;
        long long int depth = -1;
        int debug = 0;

        struct s_option opt[] = {
                {"--depth", ARG_TYPE_LLINT, (void *)&depth},
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

        cli_print_all_tlv(&dz, 0, depth, debug);

        if (dz_close(&dz) == -1) {
                LOGERROR("Failed closing dazibao.");
                status = -1;
        }

        return status;
}


int cli_compact_dz(char *file) {

        dz_t dz;
        int saved;
        int status = 0;
        if (dz_open(&dz, file, O_RDWR) < 0) {
                return -1;
        }

        saved = dz_compact(&dz);

        if (saved < 0) {
                LOGERROR("dz_compact failed.");
                status = -1;
                goto CLOSE;
        }

CLOSE:
        if (dz_close(&dz) < 0) {
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

int cli_rm_tlv(int argc, char **argv) {
        dz_t dz;
        long long int off = -1;
        int status = 0;

        struct s_option opt[] = {
                {"--offset", ARG_TYPE_LLINT, (void *)&off}
        };

        struct s_args args = {&argc, &argv, opt};

        if (jparse_args(argc, argv, &args, sizeof(opt)/sizeof(*opt)) != 0) {
                LOGERROR("jparse_args failed");
                return -1;
        }

        if (argv == NULL || off == -1) {
                LOGERROR("Wrong arguments.");
                return -1;
        }

        if (dz_open(&dz, argv[0], O_RDWR) < 0) {
                LOGERROR("dz_open failed.");
                return -1;
        }

        if (dz_rm_tlv(&dz, (off_t)off) < 0) {
                LOGERROR("dz_rm_tlv failed.");
                status = -1;
                goto CLOSE;
        }

CLOSE:
        if (dz_close(&dz) < 0) {
                LOGERROR("dz_close failed.");
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
                LOGERROR(CLI_USAGE_FMT);
                exit(EXIT_FAILURE);
        }

        cmd = argv[1];

        if (strcmp(cmd, "add") == 0) {
                if (cli_add(argc - 2, &argv[2]) != 0) {
                        LOGERROR("TLV addition failed.");
                        return EXIT_FAILURE;
                }
        } else if (strcmp(cmd, "dump_tlv") == 0) {
                if (cli_dump_tlv(argc - 2, &argv[2], STDOUT_FILENO) == -1) {
                        LOGERROR("TLV dumping failed.");
                        return EXIT_FAILURE;
                }
        } else if (strcmp(cmd, "dump") == 0) {
                if (cli_dump_dz(argc - 2, &argv[2], STDOUT_FILENO) == -1) {
                        LOGERROR("Dazibao dumping failed.");
                        return EXIT_FAILURE;
                }
        } else if (strcmp(cmd, "compact") == 0) {
                if (cli_compact_dz(argv[2]) == -1) {
                        LOGERROR("Dazibao dumping failed.");
                        return EXIT_FAILURE;
                }
        } else if (strcmp(cmd, "rm") == 0) {
                if (cli_rm_tlv(argc - 2, &argv[2]) == -1) {
                        LOGERROR("Failed removing TLV.");
                        return EXIT_FAILURE;
                }
        } else {
                LOGERROR("%s is not a valid command.", cmd);
                return EXIT_FAILURE;
        }
                return EXIT_SUCCESS;
}
