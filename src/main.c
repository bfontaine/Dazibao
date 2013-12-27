#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"
#include "mdazibao.h"
#include "main.h"

/** @file */

/** buffer size used in various functions */
#define BUFFSIZE 512

int check_option_add(int argc, char **argv, int *date_idx, int *compound_idx,
                int *dz_idx, int *type_idx, int *input_idx) {
        int idx = 0,
            args_count = 0;
        for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--type")) {
                        *type_idx = idx;
                } else if ((strcmp(argv[i], "--date") == 0)) {
                        if (*date_idx < 0) {
                                *date_idx = idx;
                        }
                } else if (strcmp(argv[i], "--dazibao") == 0) {
                        if (*dz_idx < 0) {
                                *dz_idx = idx;
                        }
                } else if (strcmp(argv[i], "--compound") == 0) {
                        if (*compound_idx < 0) {
                                *compound_idx = idx;
                        }
                } else if (strcmp(argv[i], "-") == 0) {
                        if (*input_idx < 0) {
                                *input_idx = idx;
                                argv[idx] = argv[i];
                                idx++;
                                args_count++;
                        }
                } else {
                        /* if the current parameter is not an option */
                        argv[idx] = argv[i];
                        idx++;
                        args_count++;
                }
        }
        return args_count;
}

int check_type_args(int argc, char *type_args, char *op_type, int f_dz) {
        char * delim = ",\0";
        char *tmp = strtok(op_type, delim);
        int i = 0;
        while (tmp != NULL) {
                int tmp_type = tlv_str2type(tmp);
                if (tmp_type == (char) -1) {
                        printf("unrecognized type %s\n", tmp);
                        return -1;
                } else {
                        type_args[i] = tmp_type;
                }
                tmp = strtok(NULL, delim);
                i++;
        }

        if (i != (argc + (f_dz >= 0 ? -1 : 0))) {
                printf("args to option type too large\n");
                return -1;
        }
        return 0;
}

int check_args(int argc, char **argv, int *f_dz, int *f_co, int *f_d) {
        int date_size = 0,
            compound_size = 0,
            tmp_size = 0,
            i;
        for (i = 0; i < argc; i++) {
                if (strcmp(argv[i],"-") == 0) {
                        continue;
                } else if ( i == *f_dz) {
                        if ((tmp_size = check_dz_path(argv[i], R_OK)) < 0) {
                                printf("check dz arg failed :%s\n", argv[i]);
                                return -1;
                        }
                } else if ( i >= *f_co) {
                        if ((tmp_size = check_tlv_path(argv[i], R_OK)) < 0) {
                                printf("check path arg failed :%s\n", argv[i]);
                                return -1;
                        }
                        compound_size += tmp_size;
                } else {
                        tmp_size = check_tlv_path(argv[i], R_OK);
                        if (tmp_size < 0) {
                                printf("check arg failed :%s\n", argv[i]);
                                return -1;
                        }
                }
                /* check size file for tlv */
                if ((*f_d <= *f_co) && (i >= *f_co)) {
                        date_size += compound_size +
                                TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                } else {
                        date_size += tmp_size;
                }

                if (tmp_size > TLV_MAX_VALUE_SIZE) {
                        printf("tlv too large, %s\n",argv[i]);
                        return -1;
                } else if (date_size > TLV_MAX_VALUE_SIZE) {
                        printf("tlv date too large, %s\n",argv[i]);
                        return -1;
                } else if (compound_size > TLV_MAX_VALUE_SIZE) {
                        printf("tlv compound too large, %s\n",argv[i]);
                        return -1;
                }
                tmp_size = 0;
                date_size = 0;
        }
        return 0;
}

int cmd_add(int argc, char **argv, char * daz) {
        int f_date = -1,
            f_compound = -1,
            f_dz = -1,
            f_type = -1,
            f_input = -1;
        char *type_args = NULL;

        if (argc < 0) {
                printf("error no args for add\n");
                return -1;
        }

        argc = check_option_add(argc, argv, &f_date, &f_compound, &f_dz,
                        &f_type, &f_input);

        if (f_type >= 0) {
                type_args = (char *) malloc(sizeof(*type_args)* argc);
                char *tmp = argv[f_type];
                if (f_type < (argc -1)) {
                        /* deleted type args in argv*/
                        int i;
                        for (i = (f_type + 1); i < (argc); i++) {
                                argv[i-1] = argv[i];
                        }
                        /* shift flag if it after type args */
                        f_date  = (f_date > f_type ? f_date -1 : f_date);
                        f_compound = (f_compound > f_type ? f_compound-1 :
                                        f_compound);
                        f_input = (f_input > f_type ? f_input-1 : f_input);
                        f_dz = (f_dz > f_type ? f_dz-1 : f_dz);
                }
                argc--;
                if (check_type_args(argc, type_args, tmp, f_dz) < 0) {
                        printf("check_args_no_op failed\n");
                        free(type_args);
                        return -1;
                }
        } else if ( argc > (f_dz >= 0 ? 1:0) + (f_input >= 0 ? 1:0)) {
                printf("check_type_args failed\n");
                return -1;
        }

        if (check_args(argc, argv, &f_dz, &f_compound, &f_date)) {
                printf("check path args failed\n");
                return -1;
        }

        if (action_add(argc, argv, f_compound, f_dz, f_date, f_input,
                type_args, daz) == -1) {
                printf("error action add\n");
                return -1;
        }
        if (f_type >= 0) {
                free(type_args);
        }
        return 0;
}

int action_add(int argc, char **argv, int f_co, int f_dz, int f_d, int f_in,
                char *type, char *daz) {
        dz_t daz_buf;
        unsigned int buff_size_co = 0;
        tlv_t tlv = NULL;
        tlv_t buff_co = NULL;
        tlv_t buff_d = NULL;
        int i,j = 0;

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                fprintf(stderr, "failed open the dazibao\n");
                return -1;
        }

        f_d = (f_d == -1 ? argc : f_d);
        f_co = (f_co == -1 ? argc : f_co);

        for (i = 0; i < argc; i++) {
                int tlv_size = 0;
                /* inizialized tlv */
                if (tlv_init(&tlv) < 0) {
                      printf("error to init tlv\n");
                        return -1;
                }

                /* different possibility to create a standard tlv*/
                if (i == f_in) {
                        tlv_size = tlv_create_input(&tlv, &type[j]);
                        j++;
                } else if (i == f_dz) {
                        tlv_size = dz2tlv(argv[i], &tlv);
                } else {
                        tlv_size = tlv_create_path(argv[i], &tlv, &type[j]);
                        j++;
                }

                /* if not tlv as create error */
                if (tlv_size < 0) {
                        printf("error to create tlv with path %s\n", argv[i]);
                        return -1;
                }

                /* other option who use tlv created */
                if ( i >= f_co ) {
                        /* if tlv to insert to compound it type dated*/
                        if ((i >= f_d) && (f_d > f_co)) {
                                if (tlv_init(&buff_d) < 0) {
                                        printf("error to init tlv compound");
                                        return -1;
                                }
                                tlv_size = tlv_create_date(&buff_d, &tlv,
                                                tlv_size);
                                if (tlv_size < 0) {
                                        printf("error to create tlv dated"
                                                        "%s\n", argv[i]);
                                        return -1;
                                }
                                tlv_destroy(&tlv);
                                tlv = buff_d;
                                buff_d = NULL;
                        }
                        unsigned int size_realloc = TLV_SIZEOF_HEADER;
                        if ((f_co == i) && (tlv_init(&buff_co) < 0)) {
                                printf("error to init tlv compound\n");
                                return -1;
                        }
                        size_realloc += buff_size_co + tlv_size;
                        buff_co = (tlv_t) safe_realloc(buff_co,
                                        sizeof(*buff_co) * size_realloc);
                        if (buff_co == NULL) {
                                ERROR("realloc", -1);
                        }

                        memcpy(buff_co + buff_size_co, tlv, tlv_size);
                        buff_size_co += tlv_size;
                        tlv_destroy(&tlv);

                        /*when all tlv is insert, create tlv compound*/
                        if (i == argc -1) {
                                if (tlv_init(&tlv) < 0) {
                                        printf(" error to init tlv");
                                        return -1;
                                }
                                tlv_size = tlv_create_compound(&tlv, &buff_co,
                                        buff_size_co);
                                if (tlv_size < 0) {
                                        printf(" error to create compound"
                                        " %s\n", argv[i]);
                                        return -1;
                                }
                                tlv_destroy(&buff_co);
                        } else {
                                continue;
                        }
                }

                if ((i >= f_d) && (f_d <= f_co)) {
                        if (tlv_init(&buff_d) < 0) {
                                printf(" error to init tlv dated\n");
                                return -1;
                        }
                        tlv_size = tlv_create_date(&buff_d, &tlv, tlv_size);
                        if (tlv_size < 0) {
                                printf(" error to create tlv dated"
                                        "%s\n", argv[i]);
                                return -1;
                        }
                        tlv_destroy(&tlv);
                        tlv = buff_d;
                        buff_d = NULL;
                }

                if (tlv_size > 0) {
                        if (dz_add_tlv(&daz_buf, &tlv) == -1) {
                                fprintf(stderr, "failed adding the tlv\n");
                                tlv_destroy(&tlv);
                                return -1;
                        }
                        tlv_destroy(&tlv);
                }
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "failed closing the dazibao\n");
                return -1;
        }
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
        int saved;
        if (argc > 0) {
                fprintf(stderr, "'compact' doesn't take any option\n");
                return DZ_ARGS_ERROR;
        }

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                return -1;
        }

        saved = dz_compact(&daz_buf);

        if (saved < 0) {
                return -1;
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                return -1;
        }

        printf("%d bytes saved.\n", saved);
        return 0;

}

void print_usage(char *name) {
        printf(CLI_USAGE_FMT, name);
}

/** CLI's main function */
int main(int argc, char **argv) {
        char *cmd, *daz, **argv_cmd;
        int argc_cmd, st = 0;

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
                /* shift argv to the right to remove the program name and the
                * command */
                argv_cmd = argv + 2;
        }
        /* TODO factorize this code */
        if (!strcmp(cmd, "add")) {
                st = cmd_add(argc_cmd, argv_cmd, daz);
        } else if (!strcmp(cmd, "rm")) {
                st = cmd_rm(argc_cmd, argv_cmd, daz);
        } else if (!strcmp(cmd, "dump")) {
                st = cmd_dump(argc_cmd, argv_cmd, daz);
        } else if (!strcmp(cmd, "create")) {
                st = cmd_create(argc_cmd, argv_cmd, daz);
        } else if (!strcmp(cmd, "compact")) {
                st = cmd_compact(argc_cmd, argv_cmd, daz);
        /* /factorize */
        } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }

        if (st < 0) {
                fprintf(stderr, "Error %d\n", st);
                exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
}

