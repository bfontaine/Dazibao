#include <stdio.h>
#include <assert.h>
#include "utils.h"
#include "tests.h"

INIT_TESTS();

void str2dec_positive_tests(void) {
        LONGTEST(str2dec_positive("45"), 45);
        LONGTEST(str2dec_positive("1999"), 1999);

        LONGTEST(str2dec_positive("0"), 0);

        LONGTEST(str2dec_positive("-1"), -1);
        LONGTEST(str2dec_positive("-42"), -1);

        LONGTEST(str2dec_positive("3.14"), -1);
        LONGTEST(str2dec_positive("-3.1"), -1);

        LONGTEST(str2dec_positive("foo"), -1);
        LONGTEST(str2dec_positive("]($"), -1);

        LONGTEST(str2dec_positive("0b10"), -1);
        LONGTEST(str2dec_positive("0xAF"), -1);

        LONGTEST(str2dec_positive(""), -1);
        LONGTEST(str2dec_positive(NULL), -1);
}

int main(void) {
        START_TESTS();

        ADD_SUITE(str2dec_positive);

        END_OF_TESTS();
        return 0;
}
