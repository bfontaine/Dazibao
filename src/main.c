#include "main.h"
#include "dazibao.h"
#include <locale.h>

#define BUFFSIZE 512

int cmd_add(int argc ,char **argv){
        long tmp_type;
        if (argc < 4) {
                printf("expected type\n");
                exit(EXIT_FAILURE);
        }

        tmp_type = strtol(argv[3], NULL, 10);
        if (STRTOL_ERR(tmp_type)) {
                printf("unrecognized type\n");
                exit(EXIT_FAILURE);
        }
        type = (unsigned char)tmp_type;

        if (add(daz, cmd, tmp_type) == -1) {
                printf("add error\n");
                exit(EXIT_FAILURE);
        }
        return 0;
}


int add(char *daz, char *cmd, unsigned char type) {
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
        free(buff);
}

int cmd_rm(int argc , int argv){

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

int cmd_dump(int argc , char ** argv){

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

int cmd_create(int argc , char ** argv){

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
int cmd_compact(int argc , char ** argv){
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

int helper(int msg){
        /* switch msg
                test si cmd non conforme out inconnue
                si option valide
                si argument valide dazibao ou fichier de tlv
        */
        return 0;
}

int main(int argc, char **argv) {

	dz_t daz_buf;
	char *daz, *cmd;

        if (setlocale(LC_ALL, "") == NULL) {
                PERROR("setlocale");
        }

        if (argc < 3) {
                printf("Usage:\n\t%s <dazibao> <cmd>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

//	daz = argv[1];
	cmd = argv[1];
        // cree tableau des arguments

	if ( !strcmp(cmd, "add")) {
                /* cmd reconnue add
                        recupere tableau des argument et option
                        si existant:
                        --date          -d
                        --compound      -c argument type ou nom fic
                        --dazibao       -d un seul argument type compound
                        --fichier       -f argument nom de fichier
                */
	} else if (!strcmp(cmd, "rm")) {
                /* cmd rm reconnue
                        recuperer tableau option ou arguments
                rm  <offset>  <daibao>
                */

	} else if (!strcmp(cmd, "dump")) {
                /* cmd dump valider
                commande :
                        dump <option & argument> <dazibao>
                        option :
                                --debug -d
                                --depth -D argument defaud 0

                */
	} else if (!strcmp(cmd, "create")) {
                /* cmd create accepted
                        create <option & argument> <nom du dazibao>
                option : 
                --fusion -f 2 argument <dazibao> 


                */
	} else if (!strcmp(cmd, "compact")) {
                /* cmd compact
                compact <option & argument> <dazibao>
                */
	} else {
                //appeller helper avec cmd unknow
                exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
}
