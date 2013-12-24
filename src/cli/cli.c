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
#include <limits.h>

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

                fd[i] = open(argv[i], O_RDONLY);

                if (fd[i] == -1) {
                        if (access(argv[i], F_OK) == 0) {
                                LOGERROR("Failed opening %s", argv[i]);
                                status = -1;
                                goto CLOSEFD;
                        }
                        inputs[i].len = strlen(argv[i]);
                        inputs[i].data = argv[i];
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
                        inputs[i].type = tlv_guess_type(
                                inputs[i].data,
                                inputs[i].len);
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
        };

        struct s_args args = {&nb_inputs, &inputs, opt};

        if (jparse_args(argc, argv, &args, sizeof(opt)/sizeof(*opt)) == -1) {
                LOGERROR("jparse_args failed.");
                status = -1;
                goto OUT;
        }

        if (nb_inputs < 2) {
                LOGERROR("Missing parameters (see manual).");
                status = -1;
                goto OUT;
        }

        file = inputs[--nb_inputs];

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


static int cli_print_to_file(char *buf, size_t len, off_t offset, int type) {

        int wc;
        int out_fd;
        int status = 0;
        char out_s[20];

        wc = snprintf(out_s, 20,
                "%lli.%s",
                offset,
                tlv_type2str(type));

        if (wc < 0) {
                ERROR("snprintf", -1);
        }

        out_fd = open(out_s, O_CREAT | O_EXCL | O_WRONLY, 0644);

        if (out_fd == -1) {
                ERROR("open", -1);
        }

        wc = write_all(out_fd, buf, len);

        if (wc != len) {
                LOGERROR("wrote %d bytes (expected %d)", wc, len);
                status = -1;
                goto CLOSE;
        }

CLOSE:
        close(out_fd);

        return status;
}


/**
 * precondition: next dz read will be the first TLV_LONGC
 */
static int cli_extract_ltlv(dz_t *dz, tlv_t *tlv) {

        uint32_t len;
        int type;
        char *buff;
        uint32_t write_idx = 0;
        off_t off;
        int status = 0;

        len = ltlv_real_data_length(tlv);

        type = ltlv_real_data_type(tlv);

        buff = malloc(sizeof(*buff) * len);

        if (buff == NULL) {
                ERROR("malloc", -1);
        }

        while (write_idx < len && (off = dz_next_tlv(dz, tlv)) != EOD) {
                if (tlv_get_type(tlv) == TLV_LONGC) {
                        dz_read_tlv(dz, tlv, off);
                        tlv_mdump_value(tlv, buff + write_idx);
                        write_idx += tlv_get_length(tlv);
                } else {
                        LOGERROR("Long TLV incomplete.");
                        status = -1;
                        goto FREEBUFF;
                }
        }

        if (write_idx != len) {
                LOGERROR("Read: %u, expected %u", write_idx, len);
                status = -1;
                goto FREEBUFF;
        }

        switch (type) {
        default:
                LOGERROR("Not supported yet");
                status = -1;
                goto FREEBUFF;
        };

FREEBUFF:
        free(buff);

        return status;
}


int cli_extract_tlv(dz_t *dz, off_t offset) {

        tlv_t tlv;
        int status = 0;
        int out_fd;
        /* long long int is 10 character max long
         * extension should fit in 8 characters*/
        char out_s[10 + 10];
        int wc;
        int type;

        if (tlv_init(&tlv) != 0) {
                LOGERROR("Failed initializing TLV.");
                status = -1;
                goto OUT;
        }

        switch (dz_tlv_at(dz, &tlv, offset)) {
        case -1:
        case EOD:
                LOGERROR("dz_tlv_at %d failed.", (int)offset);
                status = -1;
                goto DESTROY;
        };

        dz_read_tlv(dz, &tlv, offset);

        type = tlv_get_type(&tlv);

        switch (type) {
        case TLV_COMPOUND:
                status = cli_extract_ltlv(dz, &tlv);
        case TLV_DATED:
        case TLV_LONGH:
                LOGERROR("Not supported yet.");
                status = -1;
                goto DESTROY;
        default:
                if (cli_print_to_file(tlv_get_value_ptr(&tlv),
                                        tlv_get_length(&tlv),
                                        offset,
                                        type) == -1) {
                        LOGERROR("Printing in file failed.");
                        status = -1;
                        goto DESTROY;
                }
        };

DESTROY:
        tlv_destroy(&tlv);

OUT:
        return status;
}

int cli_extract(int argc, char **argv) {

        dz_t dz;

        int status = 0;
        char value = 0;
        char **offset;
        int nb_tlv;
        char *file;

        if (argc < 2) {
                LOGERROR("Missing parameter(s).");
                return -1;
        }

        file = argv[argc - 1];
        nb_tlv = argc - 1;
        offset = argv;

        if (dz_open(&dz, file, O_RDWR) != 0) {
                LOGERROR("Failed opening %s.", argv[0]);
                return -1;
        }


        for (int i = 0; i < nb_tlv; i++) {

                char *endptr = NULL;
                long long int off = strtoll(offset[i], &endptr, 10);
                if (endptr != NULL && *endptr != '\0') {
                        fprintf(stderr, "Invalid argument: %s\n", offset[i]);
                        status = -1;
                        goto CLOSE;
                }
                if (off == LLONG_MIN || off == LLONG_MAX) {
                        fprintf(stderr, "Overflow: %s\n", offset[i]);
                        status = -1;
                        goto CLOSE;
                }

                if (cli_extract_tlv(&dz, off) == -1) {
                        LOGERROR("cli_extract failed (%d/%d tlv extracted).",
                                i, nb_tlv);
                        status = -1;
                        goto CLOSE;
                }
        }


CLOSE:
        if (dz_close(&dz) == -1) {
                LOGERROR("Failed closing dazibao.");
                status = -1;
        }

        return status;
}

int cli_print_ltlv(dz_t *dz, tlv_t *tlv, int indent, int lvl, int debug) {
        int len, type;
        const char *type_str;
        char *buf;
        int buf_idx = 0;
        tlv_t tlv_tmp;
        off_t off;

        dz_read_tlv(dz, tlv, dz_get_offset(dz));

        len = ltlv_real_data_length(tlv);
        type = ltlv_real_data_type(tlv);

        type_str = tlv_type2str(type);

        for (int i = 0; i <= indent; i++) {
                printf("--");
        }

        printf(" @[%10li]: %8s (%d bytes)\n",
                dz_get_offset(dz),
                type_str != NULL ? type_str : "unknown",
                len);

        if ((type != TLV_COMPOUND && type != TLV_DATED) || lvl == 0) {
                goto OUT;
        }

        if (tlv_init(&tlv_tmp) == -1) {
                LOGERROR("tlv_init failed");
                goto OUT;
        }

        buf = malloc(sizeof(*buf) * (len));

        if (buf == NULL) {
                ERROR("malloc", -1);
        }

        off = dz_next_tlv(dz, &tlv_tmp);

        while (buf_idx != len) {
                off = dz_next_tlv(dz, &tlv_tmp);
                int tmp_len = tlv_get_length(&tlv_tmp);
                dz_read_tlv(dz, &tlv_tmp, off);
                memcpy(buf + buf_idx, tlv_get_value_ptr(&tlv_tmp), tmp_len);
                buf_idx += tmp_len;
        }

        dz_t dz_tmp = {-1, 0, len, 0, 0, buf};

        cli_print_dz(&dz_tmp, indent + 1, lvl - 1, debug);

        free(buf);

        if (tlv_destroy(&tlv_tmp) == -1) {
        }
OUT:
        return ltlv_get_total_length(tlv);
}

int cli_print_dz(dz_t *dz, int indent, int lvl, int debug) {

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
                const char *type_str;

                if (off == -1) {
                        return -1;
                }

                int type = tlv_get_type(&tlv);

                if ((type == TLV_PAD1 || type == TLV_PADN)
                        && !debug) {
                        continue;
                }

                if (type == TLV_LONGH) {
                        dz_set_offset(dz, off);
                        off_t next = cli_print_ltlv(
                                dz, &tlv, indent, lvl, debug);
                        dz_incr_offset(dz, next);
                        continue;
                }

                for (int i = 0; i <= indent; i++) {
                        printf("--");
                }


                int len = tlv_get_length(&tlv);

                type_str = tlv_type2str(type);
                printf(" @[%10li]: %8s (%d bytes)\n", (unsigned long)off,
                        type_str ? type_str : "unknown", len);


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
                                cli_print_dz(
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
                                cli_print_dz(
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

        cli_print_dz(&dz, 0, depth, debug);

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
        int status = 0;
        char **offset;
        int nb_tlv;
        char *file;

        if (argc < 2) {
                LOGERROR("Missing parameter(s).");
                return -1;
        }

        file = argv[argc - 1];

        offset = argv;

        nb_tlv = argc - 1;

        if (dz_open(&dz, file, O_RDWR) < 0) {
                LOGERROR("dz_open failed.");
                return -1;
        }

        for (int i = 0; i < nb_tlv; i++) {

                char *endptr = NULL;
                long long int off = strtoll(offset[i], &endptr, 10);
                if (endptr != NULL && *endptr != '\0') {
                        fprintf(stderr, "Invalid argument: %s\n", offset[i]);
                        status = -1;
                        goto CLOSE;
                }
                if (off == LLONG_MIN || off == LLONG_MAX) {
                        fprintf(stderr, "Overflow: %s\n", offset[i]);
                        status = -1;
                        goto CLOSE;
                }

                if (dz_rm_tlv(&dz, (off_t)off) < 0) {
                        LOGERROR("dz_rm_tlv failed (%d/%d tlv removed).",
                                i, nb_tlv);
                        status = -1;
                        goto CLOSE;
                }
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
        } else if (strcmp(cmd, "extract") == 0) {
                if (cli_extract(argc - 2, &argv[2]) == -1) {
                        LOGERROR("TLV extraction failed.");
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
