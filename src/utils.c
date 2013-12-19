#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "mdazibao.h"
#include "tlv.h"
#include "utils.h"

/** @file */
int check_tlv_path(const char * path, int flag_access) {
        int tlv_size = TLV_SIZEOF_HEADER;
        struct stat st_path;
        if (access(path,F_OK | flag_access) < 0) {
                printf("[utils.c|check_tlv_path]file %s not exist\n",path);
                return -1;
        }
        if (stat(path, &st_path) < 0) {
                printf("[utils.c|check_tlv_path] error stat \n");
                return -1;
        }

        if (!S_ISREG(st_path.st_mode)) {
                printf("[utils.c|check_tlv_path]file %s not regular\n",path);
                return -1;
        }

        if (st_path.st_size > TLV_MAX_VALUE_SIZE) {
                printf("[utils.c|check_tlv_path]file %s is to large\n",path);
                return -1;
        }
        tlv_size = st_path.st_size;
        return tlv_size;
}

int check_dz_path(const char * path, int flag_access) {
        int tlv_size = TLV_SIZEOF_HEADER;
        struct stat st_path;
        if (access(path,F_OK | flag_access) < 0) {
                printf("[utils.c|check_dz_path]file %s not exist\n",path);
                return -1;
        }
        if (stat(path, &st_path) < 0) {
                printf("[utils.c|check_dz_path] error stat \n");
                return -1;
        }

        if (!S_ISREG(st_path.st_mode)) {
                printf("[utils.c|check_dz_path]file %s not regular\n",path);
                return -1;
        }

        if (st_path.st_size < DAZIBAO_HEADER_SIZE) {
                printf("[utils.c|check_dz_path]file %s is to large\n",path);
                return -1;
        }
        tlv_size = st_path.st_size - DAZIBAO_HEADER_SIZE;
        return tlv_size;
}

void *safe_realloc(void *ptr, size_t size) {
        void *newptr = realloc(ptr, size);

        if (newptr == NULL) {
                free(ptr);
        }
        return newptr;
}

ssize_t write_all(int fd, char *buff, int len) {
        ssize_t wrote = 0,
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

        if (s == NULL || s[0] == '\0') {
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
                                        res->options[i].name) != 0) {
                                continue;
                        }

                        is_opt = 1;

                        switch (res->options[i].type) {
                        case ARG_TYPE_LLINT:
                                if (next_arg > argc - 2) {
                                        return -1;
                                }
                                char *endptr = NULL;
                                long long int lli = strtoll(argv[next_arg + 1],
                                                        &endptr, 10);
                                if (endptr != NULL && *endptr != '\0') {
                                        fprintf(stderr,
                                                "Invalid argument: %s\n",
                                                argv[next_arg + 1]);
                                        return -1;
                                }
                                if (lli == LLONG_MIN || lli == LLONG_MAX) {
                                        fprintf(stderr, "Overflow: %s\n",
                                                argv[next_arg + 1]);
                                        return -1;
                                }
                                *((long long int*)res->options[i].value) = lli;
                                next_arg += 2;
                                break;

                                if (next_arg > argc - 2) {
                                        return -1;
                                }
                        case ARG_TYPE_STRING:
                                if (next_arg > argc - 2) {
                                        return -1;
                                }
                                *((char**)res->options[i].value) =
                                        argv[next_arg + 1];
                                next_arg += 2;
                                break;
                        case ARG_TYPE_FLAG:
                                *((int *)res->options[i].value) = 1;
                                next_arg += 1;
                                break;
                        default:
                                fprintf(stderr, "Unknown arg type, skipping."
                                                "\n");
                                return -1;
                        }
                        break;
                }

                if (!is_opt) {
                        break;
                }
        }

        if (next_arg < argc
                && strcmp("--", argv[next_arg]) == 0) {
                next_arg++;
        }

        if (res->argc != NULL && res->argv != NULL) {
                *res->argc = argc - next_arg;
                *(res->argv) = *res->argc > 0 ?
                        &argv[next_arg] : NULL;
        }
        return 0;
}
