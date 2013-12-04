#include "main.h"
#include "dazibao.h"
#include <locale.h>

#define BUFFSIZE 512

int cmd_add(int argc, char **argv) {
        long tmp_type;
	char *daz, *cmd;
        cmd = argv[1];
	daz = argv[0];
        if (argc < 4) {
                printf("expected type\n");
                exit(EXIT_FAILURE);
        }

        tmp_type = strtol(argv[3], NULL, 10);
        if (STRTOL_ERR(tmp_type)) {
                printf("unrecognized type\n");
                exit(EXIT_FAILURE);
        }

        if (action_add(daz, cmd, tmp_type) == -1) {
                printf("add error\n");
                exit(EXIT_FAILURE);
        }
        return 0;
}


int action_add(char *daz, char *cmd, unsigned char type) {
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


        tlv_t tlv = malloc((buff_size
                                + TLV_SIZEOF_HEADER) * sizeof(*tlv));
        tlv_set_type(&tlv, type);
        tlv_set_length(&tlv, buff_size);

        memcpy(tlv_get_value_ptr(tlv), buff, buff_size);

        if (dz_add_tlv(&daz_buf, tlv) == -1) {
                printf("failed adding your tlv\n");
        }

        free(tlv);
        if (buff != NULL) {
                free(buff);
        }
        return 0;
}

int cmd_rm(int argc , char ** argv) {
	dz_t daz_buf;
	char *daz;
	daz = argv[1];

        if (argc < 4) {
                printf("expected offset\n");
                exit(EXIT_FAILURE);
        }

        if (dz_open(&daz_buf, daz, O_RDWR)) {
                exit(EXIT_FAILURE);
        }

        long off = strtol(argv[3], NULL, 10);

        if (STRTOL_ERR(off)) {
                printf("wrong offset\n");
                exit(EXIT_FAILURE);
        }

        if (dz_rm_tlv(&daz_buf, (off_t)off)) {
                printf("rm failed\n");
                dz_close(&daz_buf);
                exit(EXIT_FAILURE);
        }

        if (dz_close(&daz_buf) < 0) {
                printf("Error while closing the dazibao\n");
                exit(EXIT_FAILURE);
        }

        return 0;
}

int cmd_dump(int argc , char ** argv) {
	dz_t daz_buf;
	char *daz;
	daz = argv[1];

        if (dz_open(&daz_buf, daz, O_RDONLY)) {
                exit(EXIT_FAILURE);
        }
        if ( argc < 4 ) {
                if (dz_dump(&daz_buf)) {
                        printf("dump failed\n");
                        dz_close(&daz_buf);
                        exit(EXIT_FAILURE);
                }
        } else if (argc > 5) {
                printf("expected type\n");
                exit(EXIT_FAILURE);
        } else {
                char *cmd_dump, *depth;
                int dep;
                cmd_dump = argv[3];
                depth = argv[4];
                dep = strtol(depth, NULL, 10);

                if (STRTOL_ERR(dep)) {
                        printf("wrong depth");
                        exit(EXIT_FAILURE);
                }

                if ((!strcmp(cmd_dump, "--depth")) && (dep >= 0)) {
                        int dep = strtol(depth, NULL, 10);
                        if (STRTOL_ERR(dep)) {
                                printf("wrong depth");
                                exit(EXIT_FAILURE);
                        }
                        /* option dump compound with limited depth */
                        if (dz_dump_compound(&daz_buf, EOD, dep,0)) {
                                printf("dump_compound failed\n");
                                dz_close(&daz_buf);
                                exit(EXIT_FAILURE);
                        }

                } else {
                        printf("expected type\n");
                        exit(EXIT_FAILURE);
                }
        }

        if (dz_close(&daz_buf) < 0) {
                printf("Error while closing the dazibao\n");
                exit(EXIT_FAILURE);
        }
        return 0;

}

int cmd_create(int argc , char ** argv) {
	dz_t daz_buf;
	char *daz;
	daz = argv[1];

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
int cmd_compact(int argc , char ** argv) {
	dz_t daz_buf;
	char *daz;
	daz = argv[1];
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
        \t\t\t-d or --date : (todo)\n\t\t\t-c or --compound :(todo)\n");
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
        int argc_cmd = argc-3;
        char **argv_cmd = malloc(sizeof(char *)* argc_cmd);
        int i ;
        for (i = 0; i < argc_cmd ;i++) {
                argv_cmd[i] = argv[i+2];
        }

        /*
        TODO : management error write request
        */
	if ( !strcmp(cmd, "add")) {
                if (cmd_add(argc_cmd,argv_cmd) == -1) {
                        exit(EXIT_FAILURE);
                }
	} else if (!strcmp(cmd, "rm")) {
                if (cmd_rm(argc_cmd,argv_cmd) == -1) {
                        exit(EXIT_FAILURE);
                }
	} else if (!strcmp(cmd, "dump")) {
                if (cmd_dump(argc_cmd,argv_cmd)) {
                        exit(EXIT_FAILURE);
                }
	} else if (!strcmp(cmd, "create")) {
                if (cmd_create(argc_cmd,argv_cmd)) {
                        exit(EXIT_FAILURE);
                }
	} else if (!strcmp(cmd, "compact")) {
                if (cmd_compact(argc_cmd,argv_cmd)) {
                        exit(EXIT_FAILURE);
                }
	} else {
                print_usage();
                exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
}
