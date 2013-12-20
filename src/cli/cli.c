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

/**
 * Write a tlv in dst
 * @return written byte on success, -1 on error.
 */
off_t cli_mk_tlv(struct cli_in_info inf, int type, char *dst) {

        /**
         * FIXME:
         * - Does not respect tlv type abstraction.
         * - Useless multiple copies.
         * - Only works with input file
         */

        off_t write_idx = 0;

        if (inf.size > TLV_MAX_VALUE_SIZE) {
                char *data;


                if (inf.fd == -1) {
                        data = strdup(inf.src);
                } else {
                        data = malloc(sizeof(*data) * inf.size);

                        if (data == NULL) {
                                /* TODO */
                        }
                        int rc;
                        rc = read(inf.fd, data, inf.size);

                        if (rc == -1) {
                                /* TODO */
                        } else if (rc < inf.size) {
                                /* TODO */
                        }
                }
                
                if (type == -1) {
                        type = guess_type(data, inf.size);
                }

                write_idx = ltlv_mk_tlv(&dst, data, type, inf.size);
                free(data);
                return write_idx;
        }

        write_idx += TLV_SIZEOF_HEADER;

        if (inf.fd == -1) {
                memcpy(dst + write_idx, inf.src, inf.size);
        } else {
                int rc = read(inf.fd, dst + write_idx, inf.size);

                if (rc == -1) {
                        PERROR("read");
                        return -1;
                }
                if (rc < (int)inf.size) {
                        LOGERROR("Read %d rc, expected %d",
                                (int)rc, (int)inf.size);
                        return -1;
                }
        }

        if (type == -1 &&
                (type = guess_type(dst + write_idx, inf.size)) == -1) {
                LOGERROR("Undefined type.");
                return -1;
        }

        write_idx -= TLV_SIZEOF_HEADER;
        dst[write_idx++] = type;
        htod(inf.size, &dst[write_idx]);
        write_idx += TLV_SIZEOF_LENGTH;
        write_idx += inf.size;

        return write_idx;
}


int cli_add_all(int argc, char **argv) {

        char date = 0;
        char
                *type = NULL,
                *file = NULL,
                *delim = ",",
                *buf = NULL;
        int
                i,
                len = 0,
                cmpnd_len = 0,
                compound_idx = 0,
                write_idx = 0,
                status = 0,
                nb_lhead = 0,
                nb_lcontent = 0,
                content_idx = 0;
        struct cli_in_info *in_info;
        uint32_t timestamp;

        /* Parsing arguments. */

        struct s_option opt[] = {
                {"--date", ARG_TYPE_FLAG, (void *)&date},
                {"--type", ARG_TYPE_STRING, (void *)&type},
                {"--dazibao", ARG_TYPE_STRING, (void *)&file}
        };

        struct s_args args = {&argc, &argv, opt};

        if (jparse_args(argc, argv, &args, sizeof(opt)/sizeof(*opt)) == -1) {
                LOGERROR("jparse_args failed.");
                status = -1;
                goto OUT;
        }

        if (file == NULL) {
                LOGERROR("Missing arguments (see manual).");
                status = -1;
                goto OUT;
        }

        /* Open files.
         * Save file descriptor (or -1).
         * Save input size.
         * Get total size.
         * Get number of large tlvs headers.
         * Get number of large tlvs containers.
         * Set offset of real content */

        in_info = malloc(sizeof(*in_info) * argc);

        if (in_info == NULL) {
                PERROR("malloc");
                status = -1;
                goto OUT;
        }

        for (i = 0; i < argc; i++) {
                in_info[i].src = argv[i];
                in_info[i].fd = open(in_info[i].src = argv[i], O_RDONLY);
                if (in_info[i].fd == -1) {
                        if (access(in_info[i].src = argv[i], F_OK) == 0) {
                                LOGERROR("Failed opening %s", argv[i]);
                                status = -1;
                                goto CLOSEFD;
                        }
                        int l = strlen(argv[i]);
                        in_info[i].size = l;
                        if (l > TLV_MAX_VALUE_SIZE) {
                                nb_lhead++;
                                nb_lcontent += ltlv_nb_chunks(l);
                        }

                        len += l;
                } else {
                        struct stat st;
                        if (fstat(in_info[i].fd, &st) == -1) {
                                PERROR("fstat");
                                status = -1;
                                goto CLOSEFD;
                        }
                        if (flock(in_info[i].fd, LOCK_SH)) {
                                PERROR("flock");
                                status = -1;
                                goto CLOSEFD;
                        }

                        in_info[i].size = st.st_size;

                        if (st.st_size > TLV_MAX_VALUE_SIZE) {
                                nb_lhead++;
                                nb_lcontent += ltlv_nb_chunks(st.st_size);
                        }
                        len += st.st_size;
                }

                /* Add space for header ONLY if there will be a header
                 * (i.e. this tlv is not a long tlv) */

                if (in_info[i].size <= TLV_MAX_VALUE_SIZE) {
                        len += TLV_SIZEOF_HEADER;
                }

        }

        cmpnd_len = len;

        if (argc > 1) {
                /* If more than one input
                 * we make a compound */
                len += TLV_SIZEOF_HEADER;
                content_idx += TLV_SIZEOF_HEADER;
        }

        /* Set date and update pointers if needed. */
        if (date) {
                compound_idx += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                content_idx += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                len += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                timestamp = (uint32_t) time(NULL);
                timestamp = htonl(timestamp);
                if (timestamp == (uint32_t) -1) {
                        status = -1;
                        goto OUT;
                }
        }


        /* Add space needed by large tlv headers */
        len += nb_lcontent * TLV_SIZEOF_HEADER;
        len += nb_lhead * TLV_SIZEOF_LONGH;

        /* If we are making a compound with large content
         * we will need to include it in a large tlv
         * But we wont need space for compound header
         * as it will be included in LONGH value
         * content_idx will move TLV_SIZEOF_LONGH
         * since it will still remain a header before (TLV_LONGC) */

        if (len > TLV_MAX_SIZE && argc > 1) {
                nb_lhead++;
                len += TLV_SIZEOF_LONGH - TLV_SIZEOF_HEADER;
                content_idx += TLV_SIZEOF_LONGH;
                nb_lcontent += ltlv_nb_chunks(len);
                len += ltlv_nb_chunks(len) * TLV_SIZEOF_HEADER;
        }

        /* Same behavior with a date,
         * but compound_idx move as well */
        if (len > TLV_MAX_SIZE && date) {
                nb_lhead++;
                len += TLV_SIZEOF_LONGH - TLV_SIZEOF_HEADER;
                content_idx += TLV_SIZEOF_LONGH;
                compound_idx += TLV_SIZEOF_LONGH;
                nb_lcontent += ltlv_nb_chunks(len);
                len += ltlv_nb_chunks(len) * TLV_SIZEOF_HEADER;
        }

        /* Fill the buffer */

        buf = malloc(len);

        if (buf == NULL) {
                PERROR("malloc");
                status = -1;
                goto CLOSEFD;
        }

        write_idx = content_idx;

        for (i = 0; i < argc; i++) {
                char typ;
                if (type != NULL) {
                        char *str = i == 0 ? type : NULL;
                        char *tok = strtok(str, delim);
                        typ = tlv_str2type(tok);

                        if (typ == -1) {
                                LOGERROR("Undefined type.");
                                /* TODO: Free ressources.*/
                                return -1;
                        }
                } else {
                        typ = -1;
                }

                off_t off = cli_mk_tlv(in_info[i], typ, buf + write_idx);
                write_idx += off;
        }

        /**
         * If needed, make a compound tlv from buf
         * If needed, make a dated tlv from this compound (or buf)
         * FIXME: Really not efficient
         */

        int _len = cmpnd_len + ltlv_nb_chunks(cmpnd_len) * TLV_SIZEOF_HEADER
                + TLV_SIZEOF_LONGH;

        if (argc > 1) {
                char *cmpnd = malloc(1);
                ltlv_mk_tlv(&cmpnd,
                        buf + content_idx,
                        TLV_COMPOUND,
                        cmpnd_len);
                memcpy(buf + compound_idx, cmpnd, _len);
                free(cmpnd);
        }

        char *final;

        if (date) {
                final = malloc(1);
                ltlv_mk_tlv(&final, buf + compound_idx, TLV_DATED, _len);
        } else {
                final = buf + compound_idx;
        }

        if (cli_add_tlv(file, final) != 0) {
                status = -1;
        }

        if (date) {
                free (final);
        }


FREEBUF:
        free(buf);
CLOSEFD:
        for (i = 0; i < argc; i++) {
                if (in_info[i].fd != -1 && close(in_info[i].fd) == -1) {
                        PERROR("close");
                        status = -1;
                }
        }
        free(in_info);
OUT:
        return status;

}

int cli_add_tlv(char *file, char *buf) {

        /**
         * FIXME:
         * - tlv abstraction type not respected
         */

        int status = 0;
        dz_t dz;

        if (dz_open(&dz, file, O_RDWR) != 0) {
                LOGERROR("Failed opening %s.", file);
                status = -1;
                goto OUT;
        }

        if (dz_add_tlv(&dz, &buf) != 0) {
                LOGERROR("Failed adding TLV.");
                status = -1;
                goto CLOSE;
        }

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

        return len + ltlv_nb_chunks(len) * TLV_SIZEOF_HEADER;
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
                        dz_update_offset(dz, TLV_SIZEOF_HEADER + len + next);
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


int cli_mk_long_tlv(char *file) {
        int fd;
        int type;
        char *map;
        tlv_t tlv;
        struct stat st;
        fd = open(file, O_RDONLY);
        flock(fd, LOCK_SH);
        fstat(fd, &st);
        map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        type = guess_type(map, st.st_size);
        tlv_init(&tlv);
        ltlv_mk_tlv(&tlv, map, type, st.st_size);
        munmap(map, st.st_size);
        ltlv_fwrite(&tlv, STDOUT_FILENO);
        tlv_destroy(&tlv);
        close(fd);
        return 0;
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
                if (cli_add_all(argc - 2, &argv[2]) != 0) {
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
        }  else if (strcmp(cmd, "mk_long") == 0) {
                if (cli_mk_long_tlv(argv[2]) == -1) {
                        LOGERROR("Failed making long_tlv TLV.");
                        return EXIT_FAILURE;
                }
        } else {
                LOGERROR("%s is not a valid command.", cmd);
                return EXIT_FAILURE;
        }
                return EXIT_SUCCESS;
}
