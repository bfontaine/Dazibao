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
#include "alternate_cmd.h"

int cli_mk_tlv(tlv_t *tlv, int argc, char **argv, char *type, char date) {

        int status = -1;
        uint32_t timestamp = 0;
        struct tlv_input *inputs;
        int *fd;
        char *delim = ",";

        if (date) {
                timestamp = (uint32_t) time(NULL);
                timestamp = htonl(timestamp);
                if (timestamp == (uint32_t) -1) {
                        goto OUT;
                }
        }

        fd = calloc(argc, sizeof(*fd));

        if (fd == NULL) {
                PERROR("malloc");
                goto OUT;
        }

        inputs = calloc(argc, sizeof(*inputs));

        if (inputs == NULL) {
                PERROR("malloc");
                goto CLOSEFD;
        }

        /* prepare inputs information */

        for (int i = 0; i < argc; i++) {

                fd[i] = open(argv[i], O_RDONLY);

                if (fd[i] == -1) {
                        /* It no file exists, argv[i] is a text */
                        if (access(argv[i], F_OK) == 0) {
                                LOGERROR("Failed opening %s", argv[i]);
                                goto CLOSEFD;
                        }
                        inputs[i].len = strlen(argv[i]);
                        inputs[i].data = argv[i];
                } else {
                        struct stat st;
                        if (fstat(fd[i], &st) == -1) {
                                PERROR("fstat");
                                goto CLOSEFD;
                        }
                        if (flock(fd[i], LOCK_SH)) {
                                PERROR("flock");
                                goto CLOSEFD;
                        }
                        inputs[i].len = st.st_size;
                        inputs[i].data = mmap(NULL, inputs[i].len,
                                        PROT_READ, MAP_PRIVATE, fd[i], 0);

                        if (inputs[i].data == MAP_FAILED) {
                                PERROR("mmap");
                                goto CLOSEFD;
                        }
                }

                if (type != NULL) {
                        char *str = (i == 0 ? type : NULL);
                        char *tok = strtok(str, delim);
                        inputs[i].type = tlv_str2type(tok);

                        if (inputs[i].type == -1) {
                                LOGERROR("Undefined type.");
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
                goto CLOSEFD;
        }

        /* if no errors occured */
        status = 0;

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

        int status = -1;
        dz_t dz;
        tlv_t tlv;
        char date = 0;
        char *type = NULL;
        char *file;
        char **inputs;
        int nb_inputs = 0;
        /* Parsing arguments. */

        struct s_option opt[] = {
                {"--date", ARG_TYPE_FLAG, (void *)&date},
                {"--type", ARG_TYPE_STRING, (void *)&type},
        };

        struct s_args args = {&nb_inputs, &inputs, opt};

        if (jparse_args(argc, argv, &args, sizeof(opt)/sizeof(*opt)) == -1) {
                LOGERROR("jparse_args failed.");
                goto OUT;
        }

        if (nb_inputs < 2) {
                LOGERROR("Missing parameters (see manual).");
                goto OUT;
        }

        file = inputs[--nb_inputs];

        if (dz_open(&dz, file, O_RDWR) != 0) {
                LOGERROR("Failed opening %s.", file);
                goto OUT;
        }

        if (tlv_init(&tlv) == -1) {
                LOGERROR("tlv_init failed.");
                goto CLOSE;
        }

        if (cli_mk_tlv(&tlv, nb_inputs, inputs, type, date) == -1) {
                LOGERROR("cli_mk_tlv failed.");
                goto DESTROY;
        }


        if (dz_add_tlv(&dz, &tlv) != 0) {
                LOGERROR("Failed adding TLV.");
                goto DESTROY;
        }

        /* If no error occurred */
        status = 0;

DESTROY:
        if (tlv_destroy(&tlv) != 0) {
                LOGERROR("tlv_destroy failed");
                status = -1;
        }

CLOSE:
        if (dz_close(&dz) == -1) {
                LOGERROR("Failed closing dazibao.");
                status = -1;
        }

OUT:
        return status;
}


static int cli_write_file(char *buf, size_t len,
                        long long unsigned int offset, int type) {

        int wc;
        int out_fd;
        int status = -1;
        /* long long int is 10 character max long
         * extension should fit in 8 characters*/
        char out_s[20];
        const char *type_str = tlv_type2str(type);

        wc = snprintf(out_s, 20,
                (type_str ? "%llu.%s" : "%llu%s"),
                offset,
                (type_str ? type_str : "")
                );

        if (wc < 0) {
                ERROR("snprintf", -1);
        }

        out_fd = open(out_s, O_CREAT | O_EXCL | O_WRONLY, 0644);

        if (out_fd == -1) {
                ERROR("open", -1);
        }

        wc = write_all(out_fd, buf, len);

        if (wc != (int)len) {
                LOGERROR("wrote %d bytes (expected %d)", wc, (int)len);
                goto CLOSE;
        }

        /* if no errors occured */
        status = 0;

CLOSE:
        close(out_fd);

        return status;
}


int cli_extract_ltlv(dz_t *dz, tlv_t *tlv, int offset, int name_mod) {

        uint32_t len;
        int type;
        char *buff;
        uint32_t write_idx = 0;
        off_t off = 0;
        int status = -1;

        if (dz_read_tlv(dz, tlv, offset) != 0) {
                LOGERROR("dz_read_tlv failed");
                return -1;
        }

        len = ltlv_real_data_length(tlv);

        type = ltlv_real_data_type(tlv);

        if (dz_set_offset(dz, offset + TLV_SIZEOF_LONGH) != 0) {
                LOGERROR("dz_set_offset failed");
                return -1;
        }

        buff = dz_get_ltlv_value(dz, tlv, len);

        if (buff == NULL) {
                LOGERROR("Failed retriving long TLV value");
                goto FREEBUFF;
        }

        if (type == TLV_DATED || type == TLV_COMPOUND) {

                dz_t cmpnd = {-1,
                              0,
                              len,
                              (type == TLV_DATED ? TLV_SIZEOF_DATE : 0),
                              -1,
                              buff};

                if (cli_extract_all(&cmpnd, offset + name_mod) != 0) {
                        goto FREEBUFF;
                }
        } else if (cli_write_file(buff, len, offset + name_mod, type) == -1) {
                LOGERROR("cli_write_file failed");
                goto FREEBUFF;
        }

        /* If no error occurred */
        status = 0;

FREEBUFF:
        free(buff);

        return status;
}

int cli_extract_all(dz_t *dz, int name_mod) {

        int status = -1;
        tlv_t tlv;
        off_t off;

        if (tlv_init(&tlv) == -1) {
                LOGERROR("tlv_init");
                return -1;
        }

        while ((off = dz_next_tlv(dz, &tlv)) != EOD) {
                const char *type_str;
                int type, len;

                if (off == -1) {
                        goto DESTROY;
                }

                type = tlv_get_type(&tlv);

                if (type == TLV_PAD1 || type == TLV_PADN) {
                        continue;
                }

                if (type == TLV_LONGH) {
                        if (cli_extract_ltlv(dz, &tlv, off, name_mod) != 0) {
                                LOGERROR("cli_extract_ltlv failed");
                                goto DESTROY;
                        }
                        dz_set_offset(dz, off + ltlv_get_total_length(&tlv));
                } else if (cli_extract_tlv(dz, off, name_mod) != 0) {
                        LOGERROR("cli_extract_tlv failed");
                        goto DESTROY;
                }
        }

        /* No error occurred */
        status = 0;

DESTROY:
        if (tlv_destroy(&tlv) != 0) {
                LOGERROR("tlv_destroy failed");
                status = -1;
        }

        return status;
}

int cli_extract_tlv(dz_t *dz, off_t offset, int name_mod) {

        tlv_t tlv;
        int status = -1;
        int out_fd;
        int type;

        if (tlv_init(&tlv) != 0) {
                LOGERROR("Failed initializing TLV.");
                goto OUT;
        }

        switch (dz_tlv_at(dz, &tlv, offset)) {
        case -1:
        case EOD:
                LOGERROR("dz_tlv_at %d failed.", (int)offset);
                goto DESTROY;
        };

        type = tlv_get_type(&tlv);

        switch (type) {
        case TLV_DATED:
        case TLV_COMPOUND: {
                int len = tlv_get_length(&tlv);
                dz_t cmpnd = { -1,
                               0,
                               (offset + TLV_SIZEOF_HEADER
                                       + len),
                               (offset + TLV_SIZEOF_HEADER
                                       + (type == TLV_DATED ?
                                               TLV_SIZEOF_DATE : 0)),
                               -1,
                               dz->data};

                if (cli_extract_all(&cmpnd, offset + name_mod) != 0) {
                        goto DESTROY;
                }
                break;
        };
        case TLV_LONGH:
                if (cli_extract_ltlv(dz, &tlv, offset, name_mod) != 0) {
                        LOGERROR("cli_extract_ltlv failed");
                        goto DESTROY;
                }
                break;
        default:
                if (dz_read_tlv(dz, &tlv, offset) != 0) {
                        LOGERROR("dz_read_tlv failed");
                        goto DESTROY;
                }
                if (cli_write_file(tlv_get_value_ptr(&tlv),
                                        tlv_get_length(&tlv),
                                        offset + name_mod,
                                        type) == -1) {
                        LOGERROR("Printing in file failed.");
                        goto DESTROY;
                }
        };

        /* If no error occurred */
        status = 0;

DESTROY:
        if (tlv_destroy(&tlv) != 0) {
                LOGERROR("tlv_destroy failed");
                status = -1;
        }

OUT:
        return status;
}

int cli_extract(int argc, char **argv) {

        dz_t dz;

        int status = -1;
        char value = 0;
        char **offset;
        int nb_tlv;
        char *file;

        if (argc < 1) {
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

        if (nb_tlv == 0) {
                /* TODO: handle error */
                if (cli_extract_all(&dz, 0) != 0) {
                        LOGERROR("cli_extract_all failed ");
                }
                goto CLOSE;
        }

        for (int i = 0; i < nb_tlv; i++) {

                char *endptr = NULL;
                long long int off = strtoll(offset[i], &endptr, 10);

                if (endptr != NULL && *endptr != '\0') {
                        LOGERROR("Invalid argument: %s\n", offset[i]);
                        goto CLOSE;
                }

                if (off == LLONG_MIN || off == LLONG_MAX) {
                        LOGERROR("Overflow: %s\n", offset[i]);
                        goto CLOSE;
                }

                if (cli_extract_tlv(&dz, off, 0) == -1) {
                        LOGERROR("cli_extract failed (%d/%d tlv extracted).",
                                i, nb_tlv);
                        goto CLOSE;
                }
        }

        /* If no error occurred */
        status = 0;

CLOSE:
        if (dz_close(&dz) == -1) {
                LOGERROR("Failed closing dazibao.");
                status = -1;
        }

        return status;
}

int64_t cli_print_ltlv(dz_t *dz, tlv_t *tlv, int indent, int lvl, int debug) {
        int len, type, status = -1;
        const char *type_str;
        char *buf;
        int buf_idx = 0;
        off_t off;

        if (dz_read_tlv(dz, tlv, dz_get_offset(dz)) != 0) {
                LOGERROR("dz_read_tlv");
                goto OUT;
        }

        len = ltlv_real_data_length(tlv);
        type = ltlv_real_data_type(tlv);

        type_str = tlv_type2str(type);

        for (int i = 0; i <= indent; i++) {
                printf("--");
        }

        printf(" @[%10li]: %8s (%d bytes)\n",
                (long)dz_get_offset(dz),
                (type_str != NULL ? type_str : "unknown"),
                len);

        if ((type != TLV_COMPOUND && type != TLV_DATED) || lvl == 0) {
                status = 0;
                goto OUT;
        }

        /* Skip header */
        if (dz_set_offset(dz, dz_get_offset(dz) + TLV_SIZEOF_LONGH) != 0) {
                LOGERROR("dz_set_offset");
                goto OUT;
        }

        buf = dz_get_ltlv_value(dz, tlv, len);

        if (buf == NULL) {
                LOGERROR("dz_get_ltlv_value failed");
                goto OUT;
        }

        dz_t dz_tmp = {-1, 0, len,
                       (type == TLV_DATED ? TLV_SIZEOF_DATE : 0),
                       0, buf};

        if (cli_print_dz(&dz_tmp, indent + 1, lvl - 1, debug) != 0) {
                LOGERROR("cli_print_dz failed");
                goto FREEBUF;
        }

        /* If no error occurred */
        status = 0;

FREEBUF:
        free(buf);
OUT:
        return (status == 0 ?
                (int64_t)ltlv_get_total_length(tlv) : -1);
}

int cli_print_dz(dz_t *dz, int indent, int lvl, int debug) {

        int status = -1;
        tlv_t tlv;
        off_t off;

        if (tlv_init(&tlv) == -1) {
                LOGERROR("tlv_init");
                return -1;
        }

        while ((off = dz_next_tlv(dz, &tlv)) != EOD) {
                const char *type_str;

                if (off == -1) {
                        LOGERROR("dz_next_tlv failed");
                        goto DESTROY;
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
                        if (next == -1) {
                                LOGERROR("cli_print_ltlv failed");
                                goto DESTROY;
                        }
                        dz_set_offset(dz, off + next);
                        continue;
                }

                for (int i = 0; i <= indent; i++) {
                        printf("--");
                }

                int len = tlv_get_length(&tlv);

                type_str = tlv_type2str(type);
                printf(" @[%10li]: %8s (%d bytes)\n", (unsigned long)off,
                        (type_str ? type_str : "unknown"), len);


                if ((type != TLV_DATED && type != TLV_COMPOUND) || lvl == 0) {
                        continue;
                }

                dz_t cmpnd = { -1,
                               0,
                               (off + TLV_SIZEOF_HEADER + len),
                               (off + TLV_SIZEOF_HEADER
                                       + (type == TLV_DATED ?
                                               TLV_SIZEOF_DATE : 0)),
                               -1,
                               dz->data};

                if (cli_print_dz(&cmpnd, indent + 1, lvl - 1, debug)) {
                        goto DESTROY;
                }
        }

        /* If no error occurred */
        status = 0;

DESTROY:
        if (tlv_destroy(&tlv) != 0) {
                LOGERROR("tlv_destroy failed");
                status = -1;
        }
        return status;
}

int cli_dump_dz(int argc, char **argv) {


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

        for (int i = 0; i < argc; i++) {

                dz_t dz;

                if (argc > 1) {
                        printf("%s:\n", argv[i]);
                }

                if (dz_open(&dz, argv[i], O_RDONLY) != 0) {
                        LOGERROR("Failed opening %s.", argv[i]);
                        return -1;
                }

                if (cli_print_dz(&dz, 0, depth, debug) != 0) {
                        LOGERROR("cli_print_dz failed");
                        status = -1;
                }

                if (dz_close(&dz) == -1) {
                        LOGERROR("Failed closing dazibao.");
                        status = -1;
                }

                if (status == -1) {
                        break;
                }
        }

        return status;
}


int cli_compact_dz(int argc, char **argv) {

        int status = 0;

        for (int i = 0; i < argc; i++) {

                dz_t dz;
                int saved;

                if (dz_open(&dz, argv[i], O_RDWR) < 0) {
                        return -1;
                }

                saved = dz_compact(&dz);

                if (saved < 0) {
                        LOGERROR("dz_compact failed.");
                        status = -1;
                }

                if (dz_close(&dz) < 0) {
                        LOGERROR("dz_close failed");
                        status = -1;
                }

                if (status == 0) {
                        LOGINFO("%s: %d bytes saved.", argv[i], saved);
                } else {
                        break;
                }
        }

        return status;
}

int cli_rm_tlv(int argc, char **argv) {
        dz_t dz;
        int status = -1;
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
                        LOGERROR("Invalid argument: %s\n", offset[i]);
                        goto CLOSE;
                }
                if (off == LLONG_MIN || off == LLONG_MAX) {
                        LOGERROR("Overflow: %s\n", offset[i]);
                        goto CLOSE;
                }

                if (dz_rm_tlv(&dz, (off_t)off) < 0) {
                        LOGERROR("dz_rm_tlv failed (%d/%d tlv removed).",
                                i, nb_tlv);
                        goto CLOSE;
                }
        }

        /* If no error occurred */
        status = 0;

CLOSE:
        if (dz_close(&dz) < 0) {
                LOGERROR("dz_close failed.");
                status = -1;
                goto OUT;
        }

OUT:
        return status;
}

int cli_create_dz(int argc, char **argv) {

        for (int i = 0; i < argc; i++) {

                dz_t dz;

                if (dz_create(&dz, argv[i]) != 0) {
                        LOGERROR("dz_create failed");
                        return -1;
                }

                if (dz_close(&dz) < 0) {
                        LOGERROR("dz_close failed");
                        return -1;
                }

        }

        return 0;
}

int main(int argc, char **argv) {

        char *cmd;

        if (setlocale(LC_ALL, "") == NULL) {
                perror("setlocale");
        }

        if (argc < 3) {
                LOGERROR(CLI_USAGE_FMT, argv[0]);
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
        } else if (!strcmp(cmd, "extract_multi")) {
                if (cmd_add(argc - 3, &argv[2], argv[argc - 1]) != 0) {
                        LOGERROR("TLV addition failed.");
                        return EXIT_FAILURE;
                }
        } else if (strcmp(cmd, "dump") == 0) {
                if (cli_dump_dz(argc - 2, &argv[2]) == -1) {
                        LOGERROR("Dazibao dumping failed.");
                        return EXIT_FAILURE;
                }
        } else if (strcmp(cmd, "compact") == 0) {
                if (cli_compact_dz(argc - 2, &argv[2]) == -1) {
                        LOGERROR("Dazibao dumping failed.");
                        return EXIT_FAILURE;
                }
        } else if (strcmp(cmd, "rm") == 0) {
                if (cli_rm_tlv(argc - 2, &argv[2]) == -1) {
                        LOGERROR("Failed removing TLV.");
                        return EXIT_FAILURE;
                }
        } else if (strcmp(cmd, "create") == 0) {
                if (cli_create_dz(argc - 2, &argv[2]) == -1) {
                        LOGERROR("Failed creating dazibao.");
                        return EXIT_FAILURE;
                }
        } else if (!strcmp(cmd, "alt-add")) {
                if (cmd_add(argc - 2, &argv[2], argv[argc - 1]) != 0) {
                        LOGERROR("TLV addition failed.");
                        return EXIT_FAILURE;
                }
        } else if (!strcmp(argv[2], "alt-extract")) {
                if (cmd_extract(argc - 2, &argv[2], argv[argc - 1]) != 0) {
                        LOGERROR("Extraction failed");
                        return EXIT_FAILURE;
                }
        }  else {
                LOGERROR("%s is not a valid command.", cmd);
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
}
