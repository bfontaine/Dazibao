#include <stdio.h>
#include <stdlib.h>
#include "tlv.h"
#include "tests.h"

INIT_TESTS();

void tlv_types_tests(void) {
        char has_TLV_PAD1 = 0,
             has_TLV_PADN = 0,
             has_TLV_TEXT = 0,
             has_TLV_PNG  = 0,
             has_TLV_JPEG = 0,
             has_TLV_COMPOUND = 0,
             has_TLV_DATED = 0;

#ifdef TLV_PAD1
        has_TLV_PAD1 = 1;
#endif
#ifdef TLV_PADN
        has_TLV_PADN = 1;
#endif
#ifdef TLV_TEXT
        has_TLV_TEXT = 1;
#endif
#ifdef TLV_PNG
        has_TLV_PNG  = 1;
#endif
#ifdef TLV_JPEG
        has_TLV_JPEG = 1;
#endif
#ifdef TLV_COMPOUND
        has_TLV_COMPOUND = 1;
#endif
#ifdef TLV_DATED
        has_TLV_DATED = 1;
#endif

        INTTEST(has_TLV_PAD1, 1);
        INTTEST(has_TLV_PADN, 1);
        INTTEST(has_TLV_TEXT, 1);
        INTTEST(has_TLV_PNG, 1);
        INTTEST(has_TLV_JPEG, 1);
        INTTEST(has_TLV_COMPOUND, 1);
        INTTEST(has_TLV_DATED, 1);
}

void htod_tests(void) {
        char c[] = { 0, 0, 0 };

        CHARTEST((htod(42, c), c[2]), 42);
        CHARTEST((htod(255, c), c[2]), 255);
        CHARTEST((htod(256, c), c[1]), 1);

        CHARTEST((htod(256*256, c), c[0]), 1);
        CHARTEST((htod(256*256, c), c[1]), 0);
        CHARTEST((htod(256*256, c), c[2]), 0);

        CHARTEST((htod((256+42)*256, c), c[1]), 42);
}

void dtoh_tests(void) {
//unsigned int dtoh(char *len);
        char t_0_0_0[] = {0, 0, 0},
             t_0_0_1[] = {0, 0, 1},
             t_0_1_0[] = {0, 1, 0},
             t_1_0_0[] = {1, 0, 0},
             t_1_0_1[] = {1, 0, 1};

        UINTTEST(dtoh(t_0_0_0), 0);
        UINTTEST(dtoh(t_0_0_1), 1);
        UINTTEST(dtoh(t_0_1_0), 256);
        UINTTEST(dtoh(t_1_0_0), 256*256);
        UINTTEST(dtoh(t_1_0_1), 256*256+1);
}

void tlv_init_tests(void) {
        tlv_t initialized = NULL;
        tlv_init(&initialized);
        INTTEST(initialized != NULL, 1);
        free(initialized);
}

void tlv_destroy_tests(void) {
        tlv_t t = NULL;
        tlv_init(&t);
        INTTEST(t != NULL, 1);
        tlv_destroy(&t);
        STRTEST(t, NULL);
}

void tlv_set_get_type_tests(void) {
        tlv_t t = NULL;
        tlv_init(&t);

        INTTEST((tlv_set_type(&t,   0), tlv_get_type(&t)),  0);
        INTTEST((tlv_set_type(&t,  42), tlv_get_type(&t)), 42);

        tlv_destroy(&t);
}

void tlv_set_get_pad1_length_tests(void) {
        tlv_t pad1 = NULL;
        tlv_init(&pad1);
        tlv_set_type(&pad1, TLV_PAD1);

        UINTTEST((tlv_set_length(&pad1,    0), tlv_get_length(&pad1)), 0);
        UINTTEST((tlv_set_length(&pad1,   25), tlv_get_length(&pad1)), 0);
        UINTTEST((tlv_set_length(&pad1, 2500), tlv_get_length(&pad1)), 0);

        tlv_destroy(&pad1);
}

void tlv_set_get_length_tests(void) {
        tlv_t t = NULL;
        tlv_init(&t);
        tlv_set_type(&t, 25);

        UINTTEST((tlv_set_length(&t,    0), tlv_get_length(&t)),    0);
        UINTTEST((tlv_set_length(&t,   25), tlv_get_length(&t)),   25);
        UINTTEST((tlv_set_length(&t, 2500), tlv_get_length(&t)), 2500);

        tlv_destroy(&t);
}

void tlv_type2str_tests(void) {
        INTTEST(tlv_type2str(1)  != NULL, 1);
        INTTEST(tlv_type2str(42) != NULL, 1);
}

void tlv_str2type_tests(void) {
        CHARTEST(tlv_str2type("foo"), -1);
        CHARTEST(tlv_str2type(""),    -1);
        CHARTEST(tlv_str2type(NULL),  -1);
}

void tlv_type2str2type_tests(void) {
        char *str_TLV_PAD1 = strdup(tlv_type2str(TLV_PAD1)),
             *str_TLV_PADN = strdup(tlv_type2str(TLV_PADN)),
             *str_TLV_TEXT = strdup(tlv_type2str(TLV_TEXT)),
             *str_TLV_PNG  = strdup(tlv_type2str(TLV_PNG)),
             *str_TLV_JPEG = strdup(tlv_type2str(TLV_JPEG)),
             *str_TLV_COMPOUND = strdup(tlv_type2str(TLV_COMPOUND)),
             *str_TLV_DATED = strdup(tlv_type2str(TLV_DATED));

        INTTEST(tlv_str2type(str_TLV_PAD1), TLV_PAD1);
        INTTEST(tlv_str2type(str_TLV_PADN), TLV_PADN);
        INTTEST(tlv_str2type(str_TLV_TEXT), TLV_TEXT);
        INTTEST(tlv_str2type(str_TLV_PNG), TLV_PNG);
        INTTEST(tlv_str2type(str_TLV_JPEG), TLV_JPEG);
        INTTEST(tlv_str2type(str_TLV_COMPOUND), TLV_COMPOUND);
        INTTEST(tlv_str2type(str_TLV_DATED), TLV_DATED);

        free(str_TLV_PAD1);
        free(str_TLV_PADN);
        free(str_TLV_TEXT);
        free(str_TLV_PNG);
        free(str_TLV_JPEG);
        free(str_TLV_COMPOUND);
        free(str_TLV_DATED);
}

int main(void) {
        
        START_TESTS();

        ADD_SUITE(tlv_types);
        ADD_SUITE(htod);
        ADD_SUITE(dtoh);
        ADD_SUITE(tlv_init);
        ADD_SUITE(tlv_destroy);
        ADD_SUITE(tlv_set_get_type);
        ADD_SUITE(tlv_set_get_pad1_length);
        ADD_SUITE(tlv_set_get_length);
        ADD_SUITE(tlv_type2str);
        ADD_SUITE(tlv_str2type);
        ADD_SUITE(tlv_type2str2type);

        END_OF_TESTS();

        return 0;
}

