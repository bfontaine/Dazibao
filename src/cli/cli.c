#include "cli.h"

int add(int argc, char **argv) {
        int date = 0;
        int compound = 0;
        char *type = "gringo";

        struct s_option opt[] = {
                {"--date", ARG_TYPE_FLAG, (void *)&date},
                {"--compound", ARG_TYPE_FLAG, (void *)&compound},
                {"--type", ARG_TYPE_STRING, (void *)&type}
        };


        struct s_args args = {&argc, &argv, opt};

        if (jparse_args(argc, argv, &args, 3) != 0) {
                ERROR("parse_args", EXIT_FAILURE);
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
