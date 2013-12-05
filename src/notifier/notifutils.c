#include "notifutils.h"

uint32_t qhashmurmur3_32(const void *data, size_t nbytes)
{
	if (data == NULL || nbytes == 0) return 0;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	const int nblocks = nbytes / 4;
	const uint32_t *blocks = (const uint32_t *)(data);
	const uint8_t *tail = (const uint8_t *)(data + (nblocks * 4));

	uint32_t h = 0;

	int i;
	uint32_t k;
	for (i = 0; i < nblocks; i++) {
		k = blocks[i];

		k *= c1;
		k = (k << 15) | (k >> (32 - 15));
		k *= c2;

		h ^= k;
		h = (h << 13) | (h >> (32 - 13));
		h = (h * 5) + 0xe6546b64;
	}

	k = 0;
	switch (nbytes & 3) {
        case 3:
		k ^= tail[2] << 16;
        case 2:
		k ^= tail[1] << 8;
        case 1:
		k ^= tail[0];
		k *= c1;
		k = (k << 13) | (k >> (32 - 15));
		k *= c2;
		h ^= k;
	};

	h ^= nbytes;

	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

int parse_args(int argc, char **argv, struct s_args *res, int nb_opt) {

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
