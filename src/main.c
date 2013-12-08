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
        /* if argc = 1 means command it like that
        .dazibao add <number type TLV> daz
        */
        if (argc == 1) {
                long tmp_type;

                tmp_type = strtol(argv[0], NULL, 10);
                if (!IN_RANGE(tmp_type, 1, 256)) {
                        fprintf(stderr, "unrecognized type\n");
                        exit(EXIT_FAILURE);
                }
                if (action_add(daz, tmp_type) == -1) {
                        fprintf(stderr, "add error action_add\n");
                        exit(EXIT_FAILURE);
                }
        }
        /* option exist (date , compound or dazibo)*/
        int flag_date, flag_compound, flag_dazibao;
        flag_date = -1;
        flag_compound = -1;
        flag_dazibao = -1;
        int args = 0;
        int i;
        for (i = 0; i < argc; i++) {
                /* check args for dazibao or compound*/
                if (args > 0) {
                        /* TODO add function
                        to check fic:
                                - normaly fic
                                - exist fic
                                - exist type TLV fic
                        */
                } else if ((!strcmp(argv[i],"--date") ||
                        !strcmp(argv[i],"-d")) && (flag_date != 1)) {
                        flag_date = 1;
                } else if ((!strcmp(argv[i],"--dazibao") ||
                        !strcmp(argv[i],"-D")) && (flag_dazibao != 1)) {
                        flag_dazibao = 1;
                        /* add check to args fic tlv */
                        args = argc - i -1;
                } else if ((!strcmp(argv[i],"--compound") ||
                        !strcmp(argv[i],"-c")) && (flag_compound != 1)) {
                        flag_compound = 1;
                        /* add check to args fic dazibao */
                        args = 1;
                } else {
                        /* TODO args[i] is not option and args
                                or already use
                                ERROR
                        */
                }
        }
        if ((flag_dazibao == 1) && (flag_compound == 1)) {
                /* help message 2 flag not it the same time*/
                return -1;
        }
        return 0;
}


int action_add(char *daz, unsigned char type) {
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

        /* If the offset doesn't start with '0' or '9', it must be
         * wrong. The user probably used 'rm <dz> <offset>' instead
         * of 'rm <offset> <dz>'.
         */
        if (argv[argc - 1][0] < 48 || argv[argc - 1][0] > 57) {
                fprintf(stderr, "Usage:\n    rm <offset> <dazibao>\n");
                return -1;
        }

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                fprintf(stderr, "Error while opening the dazibao\n");
                return -1;
        }

        off = strtol(argv[argc - 1], NULL, 10);

        if (off < DAZIBAO_HEADER_SIZE) {
                fprintf(stderr, "wrong offset\n");
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
                exit(EXIT_FAILURE);
        }

        if (dz_dump(&daz_buf, EOD, flag_depth,0,flag_debug)) {
                fprintf(stderr, "dump failed\n");
                dz_close(&daz_buf);
                exit(EXIT_FAILURE);
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                exit(EXIT_FAILURE);
        }

        return 0;
}

int cmd_dump(int argc , char ** argv, char * daz) {
        int flag_debug = -1;
        int flag_depth = 0 ;
        if (argc < 4) {
                int i;
                int args = 0;
                for (i = 0; i < argc; i++) {
                        if (args > 0) {
                                flag_depth = i;
                                args--;
                        } else if ((!strcmp(argv[i],"--depth") ||
                                !strcmp(argv[i],"-d")) && (flag_depth < 1)) {
                                flag_depth = 1;
                                args = 1;
                        } else if ((!strcmp(argv[i],"--debug") ||
                                !strcmp(argv[i],"-D")) && (flag_debug != 1)) {
                                flag_debug = 1;
                        } else if ((!strcmp(argv[i],"-dD") ||
                                !strcmp(argv[i],"-Dd")) && (flag_depth < 1)
                                && (flag_debug)) {
                                flag_debug = 1;
                                flag_depth = 1;
                                args = 1;
                        } else {
                                /* TODO args[i] is not option and args
                                        or already use
                                        ERROR
                                */
                                fprintf(stderr, "[main|cmd_dump] arg failed"
                                ":%s\n",argv[i]);
                                return -1;
                        }

                }
                if ((flag_depth == 0) && (flag_debug == -1) && (argc != 0)) {
                        /*
                        error impossible to any flag is activate
                        */
                        fprintf(stderr, "cmd_dump flag failed\n");
                        return -1;
                }
                if (flag_depth > 0) {
                        flag_depth = strtol(argv[flag_depth], NULL, 10);
                        if (flag_depth < 0) {
                                fprintf(stderr, "unrecognized depth\n");
                                return -1;
                        }
                }
        } else {
                /* if argc > 4 , too many argument
                */
                fprintf(stderr, "too many arguments with dump");
                return -1;
        }

        if (action_dump(daz, flag_debug, flag_depth) == -1) {
                fprintf(stderr, "wrong depth");
                return -1;
        }
        return 0;
}

int cmd_create(int argc , char ** argv, char * daz) {
        dz_t daz_buf;
        if (argc > 0) {
                fprintf(stderr, "no argument for commande create\n");
                return -1;
        }

        if (dz_create(&daz_buf, daz)) {
                fprintf(stderr, "error during dazibao creation\n");
                exit(EXIT_FAILURE);
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                exit(EXIT_FAILURE);
        }
        return 0;

}
int cmd_compact(int argc , char ** argv, char * daz) {
        dz_t daz_buf;
        if (argc > 0) {
                fprintf(stderr, "no argument for commande compact\n");
                return -1;
        }

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                exit(EXIT_FAILURE);
        }

        if (dz_compact(&daz_buf)) {
                exit(EXIT_FAILURE);
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                exit(EXIT_FAILURE);
        }
        return 0;

}

void print_usage(char *name) {
        printf(CLI_USAGE_FMT, name);
}

int main(int argc, char **argv) {
        char *cmd, *daz;

        if (setlocale(LC_ALL, "") == NULL) {
                PERROR("setlocale");
        }

        if (argc < 3) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
        cmd = argv[1];
        daz = argv[argc - 1];

        /* recover tab option and args */
        int argc_cmd = argc - 3;
        char **argv_cmd = argv + 2;
        if (argc_cmd == 0) {
                argc_cmd = 0;
                argv_cmd = NULL;
        }
        /*
        TODO : management error write request
        */
        if ( !strcmp(cmd, "add")) {
                if (cmd_add(argc_cmd, argv_cmd, daz) == -1) {
                        fprintf(stderr, "cmd_add failed\n");
                        exit(EXIT_FAILURE);
                }
        } else if (!strcmp(cmd, "rm")) {
                if (cmd_rm(argc_cmd, argv_cmd, daz) == -1) {
                        fprintf(stderr, "cmd_rm failed\n");
                        exit(EXIT_FAILURE);
                }
        } else if (!strcmp(cmd, "dump")) {
                if (cmd_dump(argc_cmd, argv_cmd, daz)) {
                        fprintf(stderr, "cmd_dump failed\n");
                        exit(EXIT_FAILURE);
                }
        } else if (!strcmp(cmd, "create")) {
                if (cmd_create(argc_cmd, argv_cmd, daz)) {
                        fprintf(stderr, "cmd_create failed\n");
                        exit(EXIT_FAILURE);
                }
        } else if (!strcmp(cmd, "compact")) {
                if (cmd_compact(argc_cmd, argv_cmd, daz)) {
                        fprintf(stderr, "cmd_compact failed\n");
                        exit(EXIT_FAILURE);
                }
        } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
}
