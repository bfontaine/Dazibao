#include "cli.h"

int _log_level = LOG_LVL_DEBUG;

int add(int argc, char **argv) {
        int date = 0;
        int compound = 0;
        char *type = "";
        char *dazibao = "";

        struct s_option opt[] = {
                {"--date", ARG_TYPE_FLAG, (void *)&date},
                {"--compound", ARG_TYPE_FLAG, (void *)&compound},
                {"--type", ARG_TYPE_STRING, (void *)&type},
                {"--dazibao", ARG_TYPE_STRING, (void *)&dazibao}
        };


        struct s_args args = {&argc, &argv, opt};

        if (jparse_args(argc, argv, &args, 3) != 0) {
                LOGERROR("jparse_args failed");
                return EXIT_FAILURE;
        }

        int nb_ext = argc - 1;
        int nb_comma = nb_ext - 1;

        while (nb_comma --> 0) {
                if (strchr(type, DZCLI_TYPE_SEPARATOR) == NULL) {
                        LOGERROR("Wrong number of types");
                        return EXIT_FAILURE;
                }
        }

        return EXIT_SUCCESS;
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
        } else {
                return EXIT_FAILURE;
        }
}
