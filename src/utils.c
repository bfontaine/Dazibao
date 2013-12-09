#include <string.h>
#include <stdlib.h>
#include "tlv.h"
#include "utils.h"
#include "dazibao.h"

/** @file */
const char *get_tlv_type(const char *path) {
        const char *ext;
        if (path == NULL) {
                return NULL;
        }

        if ((ext = get_ext(path)) == NULL) {
                printf("no extension = no type tlv");
                return NULL;
        }

        for (unsigned int i=0; i<TLV_TYPES_COUNT; i++) {
                if (strcasecmp(tlv_types_ext[i][0], ext) == 0) {
                        printf("no extension = no type tlv");
                        return tlv_types_ext[i][1];
                }
        }
        return NULL;
}

int check_tlv_path(const char * path, int flag_access) {
        struct stat st_path;
        if (access(path,F_OK | flag_access) < 0) {
                printf("[utils.c|check_tlv_path]file %s not exist\n",path);
                return -1;
        }
        if(stat(path, &st_path) < 0) {
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
        return 0;
}

int check_dz_path(const char * path, int flag_access) {
        struct stat st_path;
        if (access(path,F_OK | flag_access) < 0) {
                printf("[utils.c|check_dz_path]file %s not exist\n",path);
                return -1;
        }
        if(stat(path, &st_path) < 0) {
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
        return 0;
}

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

        int next_arg = 1;

        while (next_arg < argc) {
                char is_opt = 0;
                for (int i = 0; i < nb_opt; i++) {
                        if (strcmp(argv[next_arg],
                                                res->options[i].name) == 0) {
                                if (next_arg > argc - 2) {
                                        fprintf(stderr,"'%s' parameter "
                                                        "is missing.\n",
                                                        res->options[i].name);
                                        return -1;
                                }
                                is_opt = 1;
                                switch (res->options[i].type) {
                                case ARG_TYPE_INT:
                                        *((int *)res->options[i].value) =
                                                str2dec_positive(
                                                        argv[next_arg + 1]);
                                        break;
                                case ARG_TYPE_STRING:
                                        res->options[i].value =
                                                argv[next_arg + 1];
                                        break;
                                default:
                                        fprintf(stderr, "Unknown arg type, "
                                                        "doing nothing.\n");
                                        return -1;
                                }


                                next_arg += 2;
                                break;
                        }
                }

                if (!is_opt) {
                        *res->argc = argc - next_arg;
                        *(res->argv) = *res->argc > 0 ? &argv[next_arg] : NULL;
                        break;
                }
        }

        return 0;
}
