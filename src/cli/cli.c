#include "cli.h"
#include <arpa/inet.h>
#include <netinet/in.h>

int _log_level = LOG_LVL_DEBUG;

static int check_types(int argc, char *types) {
        while (argc --> 0) {
                if (strchr(types, ',') == NULL) {
                        LOGERROR("Wrong number of types");
                        return -1;
                }
        }
        return 0;
}


static int get_total_size(int argc, char **argv) {
        int i, sum = 0;
        struct stat st;
        for (i = 0; i < argc; i++) {
                if (stat(argv[i], &st) == -1) {
                        ERROR("stat", -1);
                }
                sum += st.st_size + TLV_SIZEOF_HEADER;
        }
        return sum;
}

int add(int argc, char **argv) {
        int date = 0;
        int compound = 0;
        char *types = "";
        char *dazibao = "";

        struct s_option opt[] = {
                {"--date", ARG_TYPE_FLAG, (void *)&date},
                {"--compound", ARG_TYPE_FLAG, (void *)&compound},
                {"--type", ARG_TYPE_STRING, (void *)&types},
                {"--dazibao", ARG_TYPE_STRING, (void *)&dazibao}
        };


        struct s_args args = {&argc, &argv, opt};

        if (jparse_args(argc, argv, &args, 3) != 0) {
                LOGERROR("jparse_args failed");
                return EXIT_FAILURE;
        }

        if (check_types(argc - 2, types) == -1) {
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}


int mk_tlv(int argc, char **argv, int out) {

        /**
         * TODO: lock file to avoid them
         * being modified before copied
         */

        int date = 0;
        int compound = 0;
        char *types = "";
        int tlv_size = 0;

        struct s_option opt[] = {
                {"--date", ARG_TYPE_FLAG, (void *)&date},
                {"--type", ARG_TYPE_STRING, (void *)&types},
        };

        struct s_args args = {&argc, &argv, opt};

        if (jparse_args(argc, argv, &args, 1) != 0) {
                LOGERROR("jparse_args failed");
                return -1;
        }

        if (check_types(argc - 2, types) == -1) {
                return -1;
        }

        tlv_size += get_total_size(argc, argv);

        if (tlv_size < 0) {
                LOGERROR("Failed to compute TLV size.");
                return -1;
        }

        if (date) {
                tlv_size += TLV_SIZEOF_DATE;
        }

        if (argc > 1) {
                compound = 1;
                tlv_size += TLV_SIZEOF_HEADER;
        }

        if (tlv_size > TLV_MAX_SIZE) {
                LOGERROR("Too large to fit in a TLV.");
                return -1;
        }


        int buff_size = (date ? TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE : 0)
                + (compound ? TLV_SIZEOF_HEADER : 0);

        if (buff_size == 0) {
                goto NODATENOCOMP;
        }

        char *buff = malloc(sizeof(*buff) * buff_size);
        int i = 0;

        if (date) {
                time_t tim;
                if (time(&tim) == (time_t)-1) {
                        free(buff);
                        ERROR("time", -1);
                }
                uint32_t t = hotnl((uint32_t)tim);
                
                memcpy(buff, &t, sizeof(t));
                i += sizeof(t);
        }
        
        if (compound) {
                htod(tlv_size, buff + i);
                i += TLV_SIZEOF_HEADER;
        }
        
        if (write(out, buff, buff_size) == -1) {
                free(buff);
                return -1;
        }

        free(buff);

NODATENOCOMP:
        
        for (i = 0; i < argc; i++) {
                char *type = "text";//strtok(types, &DZCLI_TYPE_SEPARATOR);
                tlv_t tlv;
                tlv_init(&tlv);
                
                if (tlv_from_file(&tlv, argv[i], tlv_str2type(type)) == -1) {
                        goto FAIL;
                }
                
                if (tlv_fwrite(&tlv, out) == -1) {
                        goto FAIL;
                }
                tlv_destroy(&tlv);
        }
        return 0;

FAIL:
        return -1;
}


int main(int argc, char **argv) {

        char *cmd;

        if (setlocale(LC_ALL, "") == NULL) {
                perror("setlocale");
        }

        if (argc < 3) {
                printf(CLI_USAGE_FMT, argv[0]);
                exit(EXIT_FAILURE);
        }
        
        cmd = argv[1];

        if (strcmp(cmd, "add") == 0) {
                return add(argc - 2, &argv[2]);
        } else if (strcmp(cmd, "mk_tlv") == 0) {
                return mk_tlv(argc - 2, &argv[2], STDOUT_FILENO);
        } else {
                return EXIT_FAILURE;
        }
}
