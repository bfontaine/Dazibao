#include <stdlib.h>
#include "tlv.h"
#include "utils.h"

/** @file */
int safe_path(const char * path, int flag_access) {
        struct stat f_path;
        if (access(path,F_OK | flag_access) == -1) {
                printf("[utils.c|safe_path]file %s not exist\n",path);
                return -1;
        }
        if(stat(path, &f_path) == -1) {
                printf("[utils.c|safe_path] error stat \n");
                return -1;
        }

        if (!S_ISREG(f_path.st_mode)) {
                printf("[utils.c|safe_path]file %s not regular\n",path);
                return -1;
        }

        if (f_path.st_size > TLV_MAX_VALUE_SIZE) {
                printf("[utils.c|safe_path]file %s is to large\n",path);
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


int jparse_args(int argc, char **argv, struct s_args *res, int nb_opt) {

	int next_arg = 1;

	while (next_arg < argc) {
		char is_opt = 0;
		int i;
		for (i = 0; i < nb_opt; i++) {
			if (strcmp(argv[next_arg], res->options[i].name) == 0) {
				if (next_arg > argc - 2) {
					fprintf(stderr, "\"%s\" parameter is missing.\n",
						res->options[i].name);
					return -1;
				}
				is_opt = 1;
				switch (res->options[i].type) {
				case ARG_TYPE_INT:
					*((int *)res->options[i].value) = atoi(argv[next_arg + 1]);
					break;
				case ARG_TYPE_STRING:
					res->options[i].value = argv[next_arg + 1];
					break;
				default:
					fprintf(stderr, "Unkown arg type, doing nothing.\n");
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
