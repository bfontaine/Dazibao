#include "cli.h"

int _log_level = LOG_LVL_DEBUG;

static int check_types(int argc, char *types) {
        while (argc --> 0) {
                if (strchr(types, DZCLI_TYPE_SEPARATOR) == NULL) {
                        LOGERROR("Wrong number of types");
                        return -1;
                }
        }
        return 0;
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

static int get_total_size(int argc, char **argv) {
        int i, sum = 0;
        for (i = 0; i < argc; i++) {
                if (stat(argv[i], &st) == -1) {
                        ERROR("stat", -1);
                }
                sum += st.st_size;
        }
        return sum;
}


int mk_tlv(int argc, char **argv) {

        /**
         * TODO: lock file to avoid them
         * being modified before copied
         */

        struct stat st;
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
                return EXIT_FAILURE;
        }

        if (check_types(argc - 2, types) == -1) {
                return EXIT_FAILURE;
        }

        tlv_size += get_total_size(argc, argv);

        if (tlv_size < 0) {
                LOGERROR("Failed to compute TLV size.");
                return EXIT_FAILURE;
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
                return EXIT_FAILURE;
        }

        char *buf = (char *)calloc(tlv_size, sizeof(*buf));

        if (buf == NULL) {
                ERROR("calloc", EXIT_FAILURE);
        }

        int i, buff_fill = 0, rc = 0;
        for (i = 0; i < argc; i++) {
                int fd;
                rc = fread();
        }

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
