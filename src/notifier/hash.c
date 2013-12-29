#include "hash.h"

/** @file
 * These functions come from external sources, see hash.h.
 **/

uint32_t SuperFastHash (const char * data, size_t len) {

        uint32_t hash = len, tmp;
        int rem;

        if (data == NULL) return 0;

        rem = len & 3;
        len >>= 2;

        /* Main loop */
        for (;len > 0; len--) {
                hash  += get16bits (data);
                tmp    = (get16bits (data+2) << 11) ^ hash;
                hash   = (hash << 16) ^ tmp;
                data  += 2*sizeof (uint16_t);
                hash  += hash >> 11;
        }

        /* Handle end cases */
        switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
        }

        /* Force "avalanching" of final 127 bits */
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash += hash >> 17;
        hash ^= hash << 25;
        hash += hash >> 6;

        return hash;
}

uint32_t adler32(const void *buf, size_t buflength) {

        const uint8_t *buffer = (const uint8_t*)buf;

        uint32_t s1 = 1;
        uint32_t s2 = 0;
        size_t n;
        for (n = 0; n < buflength; n++) {
                s1 = (s1 + buffer[n]) % 65521;
                s2 = (s2 + s1) % 65521;
        }

        return (s2 << 16) | s1;
}
