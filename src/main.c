#include "main.h"

/** @file */

/** buffer size used in various functions */
#define BUFFSIZE 512
int check_option_add(int argc, char **argv, int *f_d, int *f_co, int *f_dz,
                int *f_ty, int *f_in) {
        int args_co = 0,  /* element to tlv compound */
            args_dz = 0,
            args_ty = 0,
            ad_tmp = 0,
            count_args = 0,
            i;
        for (i = 0; i < argc; i++) {
                if (!strcmp(argv[i],"--type")) {
                        *f_ty = ad_tmp;
                        args_ty = 1;
                        /* recupérer la chaine type*/
                } else if (strcmp(argv[i],"--date") == 0) {
                        *f_d = ad_tmp;
                } else if (strcmp(argv[i],"--dazibao") == 0) {
                        *f_dz = ad_tmp;
                        args_dz = 1;
                } else if (strcmp(argv[i],"--compound") == 0) {
                        *f_co = ad_tmp;
                        args_co = argc - i -1;
                } else if (strcmp(argv[i],"-") == 0) {
                        *f_in = ad_tmp;
                        argv[ad_tmp] = argv[i];
                        ad_tmp ++;
                        count_args++;
                } else if (args_ty > 0) {
                        argv[ad_tmp] = argv[i];
                        ad_tmp ++;
                        count_args++;
                } else if (args_dz > 0) { /* check args if good path */
                        argv[ad_tmp] = argv[i];
                        ad_tmp ++;
                        count_args++;
                } else if (args_co > 0) {
                        argv[ad_tmp] = argv[i];
                        ad_tmp ++;
                        count_args++;
                } else {
                        /* if argv[i] if no args to option
                           check if is a path to tlv */
                        if (ad_tmp >= 0) {
                                argv[ad_tmp] = argv[i];
                                ad_tmp++;
                        }
                }
                printf(" f_da: %2d| f_dz: %2d| f_ty:%2d | f_co: %2d |f_in :%2d"
                " |(ar_co : %2d|ar_ty : %2d|ar_dz %2d)|ad_tmp :%2d\n",
                *f_d,*f_dz,*f_ty,*f_co,*f_in,args_co,args_dz,args_ty,ad_tmp);
        }
        return count_args;
}

int cmd_add(int argc, char **argv, char * daz) {
        int f_date = -1,
            f_compound = -1,
            f_dz = -1,
            f_type = -1,
            f_input = -1,
            date_size = 0,
            compound_size = 0,
            tmp_size = 0,
            tmp_first_args = 0,
            i;
        char *type_args;

        if (argc < 0) {
                printf("[cmd_add] error no args for add\n");
                return -1;
        }

        argc = check_option_add(argc, argv, &f_date, &f_compound, &f_dz,
        &f_type, &f_input);
        for (i = 0; i < argc; i++) {
                printf("arg %d : %s\n",i,argv[i]);
        }

        /*for (i = 0; i < argc; i++) {
                if (!strcmp(argv[i],"--type")) {
                        f_type = 0;
                        args_ty = 1;
                } else if (strcmp(argv[i],"--date") == 0) {
                        f_date = i;
                } else if (strcmp(argv[i],"--dazibao") == 0) {
                        f_dz = i;
                        args_dz = 1;
                } else if (strcmp(argv[i],"--compound") == 0) {
                        f_compound = i;
                        args_co = argc - i -1;
                } else if (strcmp(argv[i],"-") == 0) {
                        f_input = i;
                } else if (args_ty > 0) {
                        type_args = argv[i];
                } else if (args_dz > 0) {
                        if ((tmp_size = check_dz_path (argv[i], R_OK)) < 0) {
                                printf("[cmd_add] check path arg failed :%s\n",
                                        argv[i]);
                                return -1;
                        }
                } else if (args_co > 0) {

                        if ((tmp_size = check_tlv_path (argv[i], R_OK)) < 0) {
                                printf("[cmd_add] check dz arg failed :%s\n",
                                        argv[i]);
                                return -1;
                        }
                        count_args_co ++;
                } else {
                         if argv[i] if no args to option
                           check if is a path to tlv
                        tmp_size = check_tlv_path (argv[i], R_OK);
                        if (tmp_size < 0) {
                                printf("[cmd_add] check arg failed :%s\n",
                                        argv[i]);
                                return -1;
                        }
                        count_args ++;
                }
                 TODO args[i] is not option valide or path
                   printf("[cmd_add] arg failed:%s\n",argv[i]);
                   return -1;

                check size of tlv
                if (f_compound > 0) {
                        compound_size += tmp_size;
                } else {
                        date_size = 0;
                }

                if (f_date >= 0) {
                        date_size = date_size + tmp_size;
                }

                printf(" f_da: %2d| f_dz: %2d| f_ty:%2d | f_co: %2d |"
                "(s_d: %6d|s_co: %6d| s_tmp: %6d)(ct_co :%2d|ct_ar :%2d)"
                "(ar_co : %2d|ar_ty : %2d|ar_dz %2d)\n",f_date,f_dz,f_type,
                f_compound,date_size,compound_size,tmp_size,count_args,
                count_args_co,args_co,args_dz,args_ty);

                if ((compound_size > TLV_MAX_VALUE_SIZE) ||
                        (date_size > (TLV_MAX_VALUE_SIZE - TLV_SIZEOF_DATE)) ||
                        (tmp_size > TLV_MAX_VALUE_SIZE)) {
                        printf("[cmd_add] tlv too large\n");
                        return -1;
                }
                tmp_size = 0;
        }*/

        printf("AF f_da: %2d| f_dz: %2d| f_ty:%2d | f_co: %2d | f_in : %2d |"
        "(s_d: %6d|s_co: %6d| s_tmp: %6d) | argc :%d\n",
        f_date,f_dz,f_type,f_compound,f_input,date_size,compound_size,
        tmp_size,argc);
        /*char **args_v = argv + tmp_first_args + 1;
        int args_c = argc - tmp_first_args - 1;

        if (action_add(args_c, args_v, f_compound, f_dz,f_date, daz) == -1) {
                printf("[cmd_add] error action add");
                return -1;
        }*/
        return 0;
}

int action_add(int argc, char **argv, int flag_compound, int flag_dazibao
        , int flag_date, char *daz) {
        dz_t daz_buf;
        unsigned int buff_size = 0;
        tlv_t tlv;
        tlv_t buff;
        if (flag_compound == 1) {
                /* read all args fic name, and create create a tlv to it
                int i;
                int size_tlv = 0;
                if (tlv_init(&tlv) < 0) {
                        printf("[tlv_create_path] error to init tlv");
                        return -1;
                }
                tlv_set_type(&tlv, (char) TLV_COMPOUND);
                for (i = 0; i < argc; i++) {
                        if (tlv_init(&tlv) < 0) {
                                printf("[tlv_create_path] error to init tlv");
                                return -1;
                        }
                        size_tlv += tlv_create_path(argv[i], &buff);
                        buff_size += size_tlv;
                        if(buff_size > TLV_MAX_VALUE_SIZE) {
                                printf("tlv compound too large\n");
                                tlv_destroy(&tlv);
                                tlv_destroy(&buff);
                                return -1;
                        }
                        tlv_destroy(&buff);
                }*/
        }

        if (flag_dazibao == 1) {
                if (tlv_init(&tlv) < 0) {
                        printf("[action_add] error to init tlv");
                        return -1;
                }
                buff_size = dz2tlv(argv[argc-1], &tlv);
        }
        if (flag_date == 1) {
                /*
                  if only flag is activate date
                        tlv to include date is oath argv[argc -1]
                  else he exist tlv compound type to var tlv
                */
                if (flag_compound == flag_dazibao) {
                        buff_size = tlv_create_path(argv[argc-1], &buff);
                        tlv_create_date(&tlv, &buff, buff_size);
                        tlv_destroy(&buff);
                }
                else {
                        buff = tlv;
                        tlv = NULL;
                        tlv_create_date(&tlv, &buff, buff_size);
                        tlv_destroy(&buff);

                }
        }

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                fprintf(stderr, "failed open the dazibao\n");
                tlv_destroy(&tlv);
                return -1;
        }

        if (dz_add_tlv(&daz_buf, &tlv) == -1) {
                fprintf(stderr, "failed adding the tlv\n");
                tlv_destroy(&tlv);
                return -1;
        }

        tlv_destroy(&tlv);
        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "failed closing the dazibao\n");
                return -1;
        }
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

        memcpy(tlv_get_value_ptr(&tlv), buff, buff_size);

        st = dz_add_tlv(&daz_buf, &tlv);
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

        if (dz_check_tlv_at(&daz_buf, off, -1,NULL) <= 0) {
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

        if (dz_dump(&daz_buf, EOD, flag_depth, 0, flag_debug)) {
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
