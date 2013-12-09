#include "main.h"
#include "dazibao.h"
#include <limits.h>
#include <locale.h>

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
                        printf("unrecognized type\n");
                        exit(EXIT_FAILURE);
                }
                if (action_no_option_add(daz, tmp_type) == -1) {
                        printf("add error action_add\n");
                        exit(EXIT_FAILURE);
                }
        }
        /* option exist (date , compound or dazibo)*/
        int flag_date, flag_compound, flag_dazibao;
        flag_date = -1;
        flag_compound = -1;
        flag_dazibao = -1;
        int args = 0;
        int tmp_first_args;
        int i;
        for (i = 0; i < argc; i++) {
                /* if option date is only option , it need args*/
                if ((i == argc-1) && (flag_date == 1) &&
                        (flag_compoud != 1) && (flag_dazibao != 1)) {
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

        if (action_add(args_c, args_v, flag_compoud, flag_dazibao
                , flag_date) == -1) {
                printf("[main|cmd_dump] error action add");
                return -1;
        }

        return 0;
}

// args 3 flag , args_t argc_t, daz
int action_add(int argc, char **argv, int flag_compound, int flag_dazibao
, int flag_date, char **daz) {
        if (flag_compound == 1) {
                /* TODO create function path -> type
                                        (path,type) -> tlv
                                        []tlv -> tlv compound
                */
        }
        if (flag_dazibao == 1) {
                /* TODO create fonction p
                        ath dazibao -> tlv compound
                */
        }
        if (flag_date == 1) {
                /* TODO create function tlv -> tlv -> date */
        }

        /* add le tlv restant */
        return 0;
}


int action_add(char *daz, unsigned char type) {
	dz_t daz_buf;
        char reader[BUFFSIZE],
             *buff = NULL;
        unsigned int buff_size = 0;
        int read_size;

        if (dz_open(&daz_buf, daz, O_RDWR)) {
                exit(EXIT_FAILURE);
        }

        while((read_size = read(STDIN_FILENO, reader, BUFFSIZE)) > 0) {

                buff_size += read_size;

                if(buff_size > TLV_MAX_VALUE_SIZE) {
                        printf("tlv too large\n");
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
                printf("failed adding your tlv\n");
        }

        free(tlv);
        free(buff);
        return 0;
}

int cmd_rm(int argc , char ** argv, char * daz) {
	dz_t daz_buf;

        if (argc != 1) {
                printf("expected offset\n");
                return -1;
        }

        if (dz_open(&daz_buf, daz, O_RDWR)) {
                printf("Error while opening the dazibao\n");
                return -1;
        }

        long off = strtol(argv[argc - 1], NULL, 10);

        if (off < DAZIBAO_HEADER_SIZE ) {
                printf("wrong offset\n");
                return -1;
        }

        if (dz_rm_tlv(&daz_buf, (off_t)off)) {
                printf("rm failed\n");
                dz_close(&daz_buf);
                return -1;
        }

        if (dz_close(&daz_buf) < 0) {
                printf("Error while closing the dazibao\n");
                return -1;
        }

        return 0;
}
int action_dump(char *daz, int flag_debug, int flag_depth) {
	dz_t daz_buf;
        if (dz_open(&daz_buf, daz, O_RDONLY)) {
                printf("open dazibao failed\n");
                exit(EXIT_FAILURE);
        }

        if (dz_dump(&daz_buf, EOD, flag_depth,0,flag_debug)) {
                printf("dump failed\n");
                dz_close(&daz_buf);
                exit(EXIT_FAILURE);
        }

        if (dz_close(&daz_buf) < 0) {
                printf("Error while closing the dazibao\n");
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
                                printf("[main|cmd_dump] arg failed"
                                ":%s\n",argv[i]);
                                return -1;
                        }

                }
                if ((flag_depth == 0) && (flag_debug == -1) && (argc != 0)) {
                        /*
                        error impossible to any flag is activate
                        */
                        printf("cmd_dump flag failed\n");
                        return -1;
                }
                if (flag_depth > 0) {
                        flag_depth = strtol(argv[flag_depth], NULL, 10);
                        if (flag_depth >= 0) {
                                printf("unrecognized depth\n");
                                return -1;
                        }
                }
        } else {
                /* if argc > 4 , too many argument
                */
                printf("too many arguments with dump");
                return -1;
        }

        if (action_dump(daz, flag_debug, flag_depth) == -1) {
                printf("wrong depth");
                return -1;
        }
        return 0;
}

int cmd_create(int argc , char ** argv, char * daz) {
	dz_t daz_buf;
        if (argc > 0) {
                printf("no argument for commande create\n");
                return -1;
        }

        if (dz_create(&daz_buf, daz)) {
                printf("error during dazibao creation\n");
                exit(EXIT_FAILURE);
        }

        if (dz_close(&daz_buf) < 0) {
                printf("Error while closing the dazibao\n");
                exit(EXIT_FAILURE);
        }
        return 0;

}
int cmd_compact(int argc , char ** argv, char * daz) {
	dz_t daz_buf;
        if (argc > 0) {
                printf("no argument for commande compact\n");
                return -1;
        }

        if (dz_open(&daz_buf, daz, O_RDWR)) {
                exit(EXIT_FAILURE);
        }

        if (dz_compact(&daz_buf)) {
                exit(EXIT_FAILURE);
        }

        if (dz_close(&daz_buf) < 0) {
                printf("Error while closing the dazibao\n");
                exit(EXIT_FAILURE);
        }
        return 0;

}

void print_usage() {
        printf("Usage:\n\t.dazibao <cmd> <option and args> <dazibao>\n");
        printf("Cmd :\n");
        printf("\tcreate  : create a empty dazibao or \
        dazibao merges into arguments\n\tcmd : create [-m or --merge ] \
        <dazibao args to merge> <dazibao>>\n\t option create:\n\
        \t\t-m or --merge :(todo)\n");
        printf("\t- add : add TLV \n\tcmd : add [-d or --date ]\
        [-c or --compound ] <tlv args> <dazibao>\n\tadd option:\n\
        \t\t\t-d or --date : (todo)\n\t\t\t-c or --compound :(todo)\n\t\t\t\
        -C or --dazibao\n");
        printf("\t- rm  : remove TLV\ncmd : rm <offset> <dazibao>\n");
        printf("\t- dump : dump Dazibaio\ncmd : dump [-d or --debug ]\
        [-D or --depth] <number depth> <dazibao>\n\tdump option :\n\
        \t\t-D or --depth : (todo)\n\t\t-d or --debug :(todo)\n");
        printf("\t- compact : compact dazibao\ncmd : compact [-r or\
        --recusive] <dazibao>\n\tcompact option :\n\
        \t\t-r or -- recursive :(todo)\n)");
}

int main(int argc, char **argv) {
	char *cmd, *daz;

        if (setlocale(LC_ALL, "") == NULL) {
                PERROR("setlocale");
        }

        if (argc < 3) {
                print_usage();
                exit(EXIT_FAILURE);
        }
	cmd = argv[1];
        daz = argv[argc -1];

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
                        printf("cmd_add failed\n");
                        exit(EXIT_FAILURE);
                }
	} else if (!strcmp(cmd, "rm")) {
                if (cmd_rm(argc_cmd, argv_cmd, daz) == -1) {
                        printf("cmd_rm failed\n");
                        exit(EXIT_FAILURE);
                }
	} else if (!strcmp(cmd, "dump")) {
                if (cmd_dump(argc_cmd, argv_cmd, daz)) {
                        printf("cmd_dump failed\n");
                        exit(EXIT_FAILURE);
                }
	} else if (!strcmp(cmd, "create")) {
                if (cmd_create(argc_cmd, argv_cmd, daz)) {
                        printf("cmd_create failed\n");
                        exit(EXIT_FAILURE);
                }
	} else if (!strcmp(cmd, "compact")) {
                if (cmd_compact(argc_cmd, argv_cmd, daz)) {
                        printf("cmd_compact failed\n");
                        exit(EXIT_FAILURE);
                }
	} else {
                print_usage();
                exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
}
