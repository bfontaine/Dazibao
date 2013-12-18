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
#include <errno.h>
#include <sys/file.h>

#include "utils.h"
#include "logging.h"
#include "tlv.h"
#include "cli.h"
#include "mdazibao.h"


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
                ptr_compound,
                write_ptr,
                status = 0;
        int *fd;
        uint32_t timestamp;

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

        /* set date and update pointers */
        if (date) {
                ptr_compound = TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                len = TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                timestamp = (uint32_t) time(NULL);
                timestamp = htonl(timestamp);
                if (timestamp == (uint32_t) -1) {
                        status = -1;
                        goto OUT;
                }
        } else {
                len = 0;
                ptr_compound = 0;
                timestamp = 0;
        }

        fd = malloc(sizeof(*fd) * argc);

        if (fd == NULL) {
                PERROR("malloc");
                status = -1;
                goto OUT;
        }

        for (i = 0; i < argc; i++) {
                fd[i] = open(argv[i], O_RDONLY);
                if (fd[i] == -1) {
                        if (access(argv[i], F_OK) == 0) {
                                LOGERROR("Failed opening %s", argv[i]);
                                status = -1;
                                goto CLOSEFD;
                        }
                        len += strlen(argv[i]);
                } else {
                        struct stat st;
                        if (fstat(fd[i], &st) == -1) {
                                PERROR("fstat");
                                status = -1;
                                goto CLOSEFD;
                        }
                        if (flock(fd[1], LOCK_SH)) {
                                PERROR("flock");
                                status = -1;
                                goto CLOSEFD;
                        }
                        len += st.st_size;
                }
                len += TLV_SIZEOF_HEADER;
        }

        if (argc > 1) {
                len += TLV_SIZEOF_HEADER;
        }

        buf = malloc(len);

        if (buf == NULL) {
                PERROR("malloc");
                status = -1;
                goto CLOSEFD;
        }

        write_ptr = 0;

        if (argc > 1) {
                write_ptr += TLV_SIZEOF_HEADER;
        }

        if (date) {
                write_ptr += TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
        }

        for (i = 0; i < argc; i++) {

                char typ;
                unsigned int tlv_len;

                write_ptr += TLV_SIZEOF_HEADER;

                if (fd[i] == -1) {
                        tlv_len = strlen(argv[i]);
                        memcpy(buf + write_ptr, argv[i], tlv_len);
                } else {
                        tlv_len = read(fd[i], buf + write_ptr, len - write_ptr);
                        if (tlv_len == (unsigned int)-1) {
                                PERROR("read");
                                status = -1;
                                goto FREEBUF;
                        }
                }

                if (type != NULL) {
                        typ = tlv_str2type(strtok((i == 0 ? type : NULL),
                                                                delim));
                } else {
                        typ = guess_type(buf + write_ptr, len);
                }
                
                if (typ == -1) {
                        LOGERROR("Undefined type.");
                        status = -1;
                        goto FREEBUF;
                }

                write_ptr -= TLV_SIZEOF_HEADER;
                buf[write_ptr++] = typ;
                htod(tlv_len, &buf[write_ptr]);
                write_ptr += TLV_SIZEOF_LENGTH;
                write_ptr += tlv_len;

        }

        if (timestamp != 0) {
                buf[0] = TLV_DATED;
                htod(len - 1, &buf[1]);
                memcpy(&buf[1 + TLV_SIZEOF_LENGTH], &timestamp, sizeof(timestamp));
        }

        if (argc > 1) {
                buf[ptr_compound] = TLV_COMPOUND;
                htod(len - ptr_compound - TLV_SIZEOF_HEADER, &buf[ptr_compound + 1]);
        }
        
        if (cli_add_tlv(file, buf) != 0) {
                status = -1;
        }

FREEBUF:
        free(buf);
CLOSEFD:
        for (i = 0; i < argc; i++) {
                if (fd[i] != -1 && close(fd[i]) == -1) {
                        PERROR("close");
                        status = -1;
                }
        }
        free(fd);
OUT:
        return status;

}

int cli_add_tlv(char *file, char *buf) {

        int status = 0;
        tlv_t tlv;
        dz_t dz;

        if (tlv_init(&tlv) != 0) {
                LOGERROR("Failed initializing TLV.");
                status = -1;
                goto OUT;
        }

        tlv_set_type(&tlv, buf[0]);
        tlv_set_length(&tlv, dtoh(&buf[1]));

        if (dz_open(&dz, file, O_RDWR) != 0) {
                LOGERROR("Failed opening %s.", file);
                status = -1;
                goto DESTROY;
        }

        if (tlv_mread(&tlv, &buf[TLV_SIZEOF_HEADER]) == -1) {
                LOGERROR("tlv_from_file failed.");
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

int cli_dump_dz(int argc, char **argv, int out) {
        
        dz_t dz;
        int status = 0;
        long long int depth = 0;
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
        } else {
                LOGERROR("%s is not a valid command.", cmd);
                return EXIT_FAILURE;
        }
                return EXIT_SUCCESS;
}
