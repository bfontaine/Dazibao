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
                if (action_no_option_add(daz, tmp_type) == -1) {
                        fprintf(stderr, "add error action_add\n");
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
                        if (safe_path(argv[i],0)) {
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
        unsigned char type;
        unsigned int buff_size = 0;
        tlv_t *tlv = NULL;;
        tlv_t *tlv_compound;
        tlv_t *tlv_date;
        if (flag_compound == 1) {
                /* read all args fic name, and create create a tlv to it
                 TODO function int tlv_create(char **path, tlv_t tlv*)
                */
                tlv_compound = malloc(TLV_SIZEOF_HEADER + (argc * sizeof(*tlv)));
                int i;
                for (i = 0; i < argc; i++) {
                       /*buff_size += tlv_create_path(argv[i], &tlv);
                       tlv_compound[i] = tlv;
                       tlv = NULL;

                        if(buff_size > TLV_MAX_VALUE_SIZE) {
                                printf("tlv too large\n");
                                exit(EXIT_FAILURE);
                        }
                        */
                }
                /* we have all tlv to include to compound in tlv_compound []
                create function TODO : int
                tlv_create_compound(tlv[],size_compound)
                */
                /*before tlv_compound -> to tlv */
                free(tlv_compound);
        }
        if (flag_dazibao == 1) {
                /* TODO create fonction path dazibao -> tlv compound
                */
        }
        if (flag_date == 1) {
                /* TODO create function tlv -> tlv -> date */
        }

        /* add le tlv restant */
        return 0;
}

int action_no_option_add(char *daz, unsigned char type) {
	dz_t daz_buf;
        char reader[BUFFSIZE],
             *buff = NULL;
        unsigned int buff_size = 0;
        int read_size;

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

                if(!buff) {
                        PERROR("realloc");
                        exit(EXIT_FAILURE);
                }

                memcpy(buff + (buff_size - read_size),
                                reader, read_size);
        }


        tlv_t tlv = malloc((buff_size + TLV_SIZEOF_HEADER) * sizeof(*tlv));
        tlv_set_type(&tlv, type);
        tlv_set_length(&tlv, buff_size);

        memcpy(tlv_get_value_ptr(tlv), buff, buff_size);

        if (dz_add_tlv(&daz_buf, tlv) == -1) {
                fprintf(stderr, "failed adding the tlv\n");
                free(tlv);
                free(buff);
                return -1;
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
                return -1;
        }

        /* If the offset doesn't start with a character between '0' and '9', it
         * must be wrong. The user probably used 'rm <dz> <offset>' instead of
         * 'rm <offset> <dz>'.
         */
        if (argv[argc - 1][0] < 48 || argv[argc - 1][0] > 57) {
                fprintf(stderr, "Usage:\n    rm <offset> <dazibao>\n");
                return -1;
        }

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                fprintf(stderr, "Error while opening the dazibao\n");
                return -1;
        }

        off = str2dec_positive(argv[argc - 1]);

        if (off < DAZIBAO_HEADER_SIZE) {
                fprintf(stderr, "wrong offset\n");
                return -1;
        }

        if (dz_check_tlv_at(&daz_buf, off, -1) <= 0) {
                fprintf(stderr, "no such TLV\n");
                dz_close(&daz_buf);
                return -1;
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
                /* if argc > 4 , too many argument */
                fprintf(stderr, "too many arguments with dump");
                return -1;
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
                        fprintf(stderr, "[main|cmd_dump] arg failed"
                                ":%s\n", argv[i]);
                        return -1;
                }

        }
        if (!flag_depth && !flag_debug && argc != 0) {
                /*
                error impossible to any flag is activate
                */
                fprintf(stderr, "cmd_dump flag failed\n");
                return -1;
        }
        if (flag_depth > 0) {
                flag_depth = str2dec_positive(argv[(unsigned char)flag_depth]);
                if (flag_depth < 0) {
                        fprintf(stderr, "unrecognized depth\n");
                        return -1;
                }
        }

        return action_dump(daz, flag_debug, flag_depth);
}

int cmd_create(int argc, char **argv, char *daz) {
        dz_t daz_buf;
        if (argc > 0) {
                fprintf(stderr, "'create' doesn't take arguments for now\n");
                return -1;
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
                fprintf(stderr, "'compact' doesn't take any argument\n");
                return -1;
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
                PERROR("setlocale");
        }

        /* <executable> <command> <dazibao name>  = 3 args */
        if (argc < 3) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
        cmd = argv[1];
        daz = argv[argc - 1];

        /* recover tab option and args */
        argc_cmd = argc - 3;
        argv_cmd = argv + 2;
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
