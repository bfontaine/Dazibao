#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils.h"
#include "main.h"
#include "dazibao.h"

/** @file */

/** buffer size used in various functions */
#define BUFFSIZE 512

int cmd_add(int argc, char **argv, char * daz) {
        int flag_date = -1,
            flag_compound = -1,
            flag_dazibao = -1,
            args = 0,
            tmp_first_args,
            i;
        /* if argc = 1 means command it like that
        .dazibao add <number type TLV> daz
        */
        if (argc == 1) {
                long tmp_type = str2dec_positive(argv[0]);
                if (!IN_RANGE(tmp_type, 1, 256)) {
                        fprintf(stderr, "unrecognized type\n");
                        return DZ_ARGS_ERROR;
                }

                if (action_no_option_add(daz, tmp_type) < 0) {
                        fprintf(stderr, "action_add error\n");
                        return -1;
                }
        }
        for (i = 0; i < argc; i++) {
                /* if option date is only option , it need args*/
                if ((i == argc-1) && (flag_date == 1) &&
                        (flag_compound != 1) && (flag_dazibao != 1)) {
                        args = 1;
                }
                /* check args for dazibao or compound*/
                if (args > 0) {
                        if (flag_compound == 1 &&
                                check_tlv_path (argv[i], R_OK) < 0) {
                                printf("[main|cmd_dump] arg failed:%s\n",
                                        argv[i]);
                                return -1;
                        }
                        if (flag_dazibao == 1 &&
                                check_dz_path (argv[i], R_OK) < 0) {
                                printf("[main|cmd_dump] arg failed:%s\n",
                                        argv[i]);
                                return -1;
                        }
                } else if ((!strcmp(argv[i],"--date") ||
                        !strcmp(argv[i],"-d")) && (flag_date != 1)) {
                        flag_date = 1;
                } else if ((!strcmp(argv[i],"--dazibao") ||
                        !strcmp(argv[i],"-D")) && (flag_dazibao != 1)) {
                        flag_dazibao = 1;
                        /* add check to args fic tlv */
                        tmp_first_args = i;
                        args = 1;
                } else if ((!strcmp(argv[i],"--compound") ||
                        !strcmp(argv[i],"-c")) && (flag_compound != 1)) {
                        flag_compound = 1;
                        /* add check to args fic dazibao */
                        tmp_first_args = i;
                        args = argc - i -1;
                } else {
                        /* TODO args[i] is not option and args
                                or already use
                                ERROR
                        */
                        printf("[main|cmd_dump] arg failed:%s\n",argv[i]);
                        return -1;
                }
        }
        if ((flag_dazibao == 1) && (flag_compound == 1)) {
                /* help message 2 flag not it the same time*/
                printf("[main|cmd_dump] flag compound and dazibao not at the"
                "same time\n");
                return -1;
        }

        char **args_v = argv + tmp_first_args + 1;
        int args_c = argc - tmp_first_args - 1;

        if (action_add(args_c, args_v, flag_compound, flag_dazibao,
                flag_date, daz) == -1) {
                printf("[main|cmd_dump] error action add");
                return -1;
        }

        return 0;
}

int action_add(int argc, char **argv, int flag_compound, int flag_dazibao
        , int flag_date, char *daz) {
        dz_t daz_buf;
        unsigned int buff_size = 0;
        tlv_t tlv = NULL;;
        tlv_t buff;
        if (flag_compound == 1) {
                /* read all args fic name, and create create a tlv to it */
                int i;
                int size_tlv = 0;
                for (i = 0; i < argc; i++) {
                        size_tlv += tlv_create_path(argv[i], tlv);
                        buff_size += size_tlv;
                        if(buff_size > TLV_MAX_VALUE_SIZE) {
                                printf("tlv compound too large\n");
                                tlv_destroy(&tlv);
                                tlv_destroy(&buff);
                                return -1;
                        }
                        memcpy(buff + (buff_size - size_tlv),
                                tlv, buff_size);
                        tlv_destroy(&tlv);
                }
                /* we have all tlv to include to compound in tlv_compound []*/
                tlv = tlv_create_compound(buff, buff_size);
                /*before tlv_compound -> to tlv */
                tlv_destroy(&buff);
        }

        if (flag_dazibao == 1) {
                buff_size = tlv_create_daz(argv[argc-1], buff);
                tlv = tlv_create_compound(buff, buff_size);
                tlv_destroy(&buff);
        }
        if (flag_date == 1) {
                /* TODO create function tlv -> tlv -> date */
        }

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                exit(EXIT_FAILURE);
        }

        /* add le tlv restant */
        if (dz_add_tlv(&daz_buf, tlv) == -1) {
                fprintf(stderr, "failed adding the tlv\n");
                tlv_destroy(&tlv);
                return -1;
        }
        tlv_destroy(&tlv);
        return 0;
}

int action_no_option_add(char *daz, unsigned char type) {
        dz_t daz_buf;
        char reader[BUFFSIZE],
             *buff = NULL;
        unsigned int buff_size = 0;
        int read_size, st;

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                exit(EXIT_FAILURE);
        }

        while((read_size = read(STDIN_FILENO, reader, BUFFSIZE)) > 0) {

                buff_size += read_size;

                if(buff_size > TLV_MAX_VALUE_SIZE) {
                        fprintf(stderr, "tlv too large\n");
                        exit(EXIT_FAILURE);
                }

                buff = safe_realloc(buff, sizeof(*buff) * buff_size);

                if(buff == NULL) {
                        perror("realloc");
                        return DZ_MEMORY_ERROR;
                }

                memcpy(buff + (buff_size - read_size), reader, read_size);
        }


        tlv_t tlv = malloc((buff_size + TLV_SIZEOF_HEADER) * sizeof(*tlv));
        tlv_set_type(&tlv, type);
        tlv_set_length(&tlv, buff_size);

        memcpy(tlv_get_value_ptr(tlv), buff, buff_size);

        st = dz_add_tlv(&daz_buf, tlv);
        if (st < 0) {
                fprintf(stderr, "failed adding the tlv\n");
                free(tlv);
                free(buff);
                return st;
        }

        free(tlv);
        free(buff);
        return 0;
}

int cmd_rm(int argc, char **argv, char *daz) {
        dz_t daz_buf;
        long off;

        if (argc != 1) {
                fprintf(stderr, "expected offset\n");
                return DZ_ARGS_ERROR;
        }

        /* If the offset doesn't start with a character between '0' and '9', it
         * must be wrong. The user probably used 'rm <dz> <offset>' instead of
         * 'rm <offset> <dz>'.
         */
        if (argv[argc - 1][0] < 48 || argv[argc - 1][0] > 57) {
                fprintf(stderr, "Usage:\n    rm <offset> <dazibao>\n");
                return DZ_ARGS_ERROR;
        }

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                fprintf(stderr, "Error while opening the dazibao\n");
                return -1;
        }

        off = str2dec_positive(argv[argc - 1]);

        if (off < DAZIBAO_HEADER_SIZE) {
                fprintf(stderr, "wrong offset\n");
                return DZ_OFFSET_ERROR;
        }

        if (dz_check_tlv_at(&daz_buf, off, -1) <= 0) {
                fprintf(stderr, "no such TLV\n");
                dz_close(&daz_buf);
                return DZ_OFFSET_ERROR;
        }

        if (dz_rm_tlv(&daz_buf, (off_t)off)) {
                fprintf(stderr, "rm failed\n");
                dz_close(&daz_buf);
                return -1;
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                return -1;
        }

        return 0;
}

int action_dump(char *daz, int flag_debug, int flag_depth) {
        dz_t daz_buf;
        if (dz_open(&daz_buf, daz, O_RDONLY) < 0) {
                fprintf(stderr, "open dazibao failed\n");
                return -1;
        }

        if (dz_dump(&daz_buf, EOD, flag_depth,0,flag_debug)) {
                fprintf(stderr, "dump failed\n");
                dz_close(&daz_buf);
                return -1;
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                return -1;
        }

        return 0;
}

int cmd_dump(int argc , char **argv, char *daz) {
        char flag_debug = 0,
             flag_depth = 0;
        int args = 0;
        if (argc >= 4) {
                fprintf(stderr, "too many arguments with dump");
                return DZ_ARGS_ERROR;
        }
        for (int i = 0; i < argc; i++) {
                if (args > 0) {
                        flag_depth = i;
                        args--;
                } else if ((!strcmp(argv[i], "--depth") ||
                                        !strcmp(argv[i], "-d"))
                                && flag_depth < 1) {
                        flag_depth = 1;
                        args = 1;
                } else if ((!strcmp(argv[i], "--debug") ||
                        !strcmp(argv[i], "-D")) && !flag_debug) {
                        flag_debug = 1;
                /* FIXME this is UGLY code and won't work if we add
                 * more options */
                } else if ((!strcmp(argv[i],"-dD") || !strcmp(argv[i],"-Dd"))
                                && (flag_depth < 1) && !flag_debug) {
                        flag_debug = 1;
                        flag_depth = 1;
                        args = 1;
                } else {
                        /* TODO args[i] is not option and args
                                or already use
                                ERROR
                        */
                        fprintf(stderr, "[main|cmd_dump] arg failed:%s\n",
                                        argv[i]);
                        return -1;
                }

        }
        if (flag_depth > 0) {
                flag_depth = str2dec_positive(argv[(unsigned char)flag_depth]);
                if (flag_depth < 0) {
                        fprintf(stderr, "unrecognized depth\n");
                        return DZ_ARGS_ERROR;
                }
        }

        return action_dump(daz, flag_debug, flag_depth);
}

int cmd_create(int argc, char **argv, char *daz) {
        dz_t daz_buf;
        if (argc > 0) {
                fprintf(stderr, "'create' doesn't take options\n");
                return DZ_ARGS_ERROR;
        }

        if (dz_create(&daz_buf, daz) != 0) {
                fprintf(stderr, "error during dazibao creation\n");
                return -1;
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                return -1;
        }

        return 0;
}
int cmd_compact(int argc , char **argv, char *daz) {
        dz_t daz_buf;
        if (argc > 0) {
                fprintf(stderr, "'compact' doesn't take any option\n");
                return DZ_ARGS_ERROR;
        }

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                return -1;
        }

        if (dz_compact(&daz_buf)) {
                return -1;
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                return -1;
        }
        return 0;

}

void print_usage(char *name) {
        printf(CLI_USAGE_FMT, name);
}

int main(int argc, char **argv) {
        char *cmd, *daz, **argv_cmd;
        int argc_cmd;

        if (setlocale(LC_ALL, "") == NULL) {
                perror("setlocale");
        }

        /* <executable> <command> <dazibao name>  = 3 args */
        if (argc < 3) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
        cmd = argv[1];
        daz = argv[argc - 1];

        /* recover tab option and args */
        if (argc == 3) {
                argc_cmd = 0;
                argv_cmd = NULL;
        } else {
                argc_cmd = argc - 3;
                argv_cmd = argv + 2;
        }
        /*
        TODO : management error write request
        */
        if (!strcmp(cmd, "add")) {
                if (cmd_add(argc_cmd, argv_cmd, daz) < 0) {
                        fprintf(stderr, "cmd_add failed\n");
                        exit(EXIT_FAILURE);
                }
        } else if (!strcmp(cmd, "rm")) {
                if (cmd_rm(argc_cmd, argv_cmd, daz) < 0) {
                        fprintf(stderr, "cmd_rm failed\n");
                        exit(EXIT_FAILURE);
                }
        } else if (!strcmp(cmd, "dump")) {
                if (cmd_dump(argc_cmd, argv_cmd, daz)) {
                        fprintf(stderr, "cmd_dump failed\n");
                        exit(EXIT_FAILURE);
                }
        } else if (!strcmp(cmd, "create")) {
                if (cmd_create(argc_cmd, argv_cmd, daz) < 0) {
                        fprintf(stderr, "cmd_create failed\n");
                        exit(EXIT_FAILURE);
                }
        } else if (!strcmp(cmd, "compact")) {
                if (cmd_compact(argc_cmd, argv_cmd, daz) < 0) {
                        fprintf(stderr, "cmd_compact failed\n");
                        exit(EXIT_FAILURE);
                }
        } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
}
