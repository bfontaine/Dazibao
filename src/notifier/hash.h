#ifndef _HASH_C
#define _HASH_C 1
#include <stdint.h>
#include <stdlib.h>
#include "../utils.h"
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)        \
        || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)        \
                        +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

/**
 * Imported from http://www.azillionmonkeys.com/qed/hash.html
 * Seems to be fast AND reliable on large file.
 * @param data data you want to hash
 * @param len length of data
 * @return hashcode for data, 0 if data is NULL 
 */
uint32_t SuperFastHash (const char * data, size_t len);

/**
 * Grab on a random conversation about hash on stackoveflow.com
 * Slower than SuperFastHash, seems reliable.
 * @param buf data you want to hash
 * @param buflength length of data
 * @return hascode for buf
 */
uint32_t adler32(const void *buf, size_t buflength);
#endif
