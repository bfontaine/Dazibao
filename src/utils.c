#include <string.h>
#include <stdlib.h>
#include "utils.h"

/** @file */

void *safe_realloc(void *ptr, size_t size) {
        void *newptr = realloc(ptr, size);

        if (newptr == NULL) {
                free(ptr);
        }
        return newptr;
}

int write_all(int fd, char *buff, int len) {
        int wrote = 0,
            w;

        while (len > 0 && (w = write(fd, buff+wrote, len)) > 0) {
                wrote += w;
                len -= w;
        }

        return wrote;
}

long str2dec_positive(char *s) {
        char *p = NULL;
        long ret;

        if (s == NULL) {
                return -1;
        }

        ret = strtol(s, &p, 10);

        if (p != NULL && p[0] != '\0') {
                return -1;
        }

        return ret >= 0 ? ret : -1;
}

const char *get_ext(const char *path) {
        const char *dot;
        if (path == NULL) {
                return NULL;
        }

        if ((dot = strrchr(path, '.')) == NULL) {
                return NULL;
        }
        return dot + 1;
}

int jparse_args(int argc, char **argv, struct s_args *res, int nb_opt) {

        int next_arg = 0;

        while (next_arg < argc) {
                char is_opt = 0;
                for (int i = 0; i < nb_opt; i++) {
                        if (strcmp(argv[next_arg],
                                        res->options[i].name) == 0) {
                                is_opt = 1;
                                switch (res->options[i].type) {
                                case ARG_TYPE_INT:
                                        if (next_arg > argc - 2) {
                                                return -1;
                                        }
                                        *((int *)res->options[i].value) =
                                                str2dec_positive(
                                                        argv[next_arg + 1]);
                                        next_arg += 2;
                                        break;
                                case ARG_TYPE_STRING:
                                        if (next_arg > argc - 2) {
                                                return -1;
                                        }
                                        *((char **)res->options[i].value) = argv[next_arg + 1];
                                        next_arg += 2;
                                        break;
                                case ARG_TYPE_FLAG:
                                        *((int *)res->options[i].value) = 1;
                                        next_arg += 1;
                                        break;
                                default:
                                        fprintf(stderr, "Unknown arg type, "
                                                        "doing nothing.\n");
                                        return -1;
                                }
                                break;
                        }
                }

                if (!is_opt) {
                        if (res->argc != NULL) {
                                *res->argc = argc - next_arg;
                        }
                        if (res->argv != NULL) {
                                *(res->argv) = *res->argc > 0 ? &argv[next_arg] : NULL;}
                        break;
                }
        }

        return 0;
}
