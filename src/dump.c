#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "dump.h"

int main(int argc, char **argv) {

        struct dazibao d;
        struct tlv t;
        off_t off;
        int len;

        if (argc < 2) {
                printf("Usage:\n\t%s <dazibao>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        open_dazibao(&d, argv[1], O_RDONLY);

        while ((off = next_tlv(&d, &t)) != EOD) {

                len = t.type == TLV_PAD1 ? 0 : t.length;

                printf("[%4d] TLV %3d | %8d | ...\n", (int)off, t.type, len);

        }

        close_dazibao(&d);
        return 0;
}
