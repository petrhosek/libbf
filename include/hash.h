#ifndef HASH_H_
#define HASH_H_ 1

#include <stdint.h>
#include <unistd.h>

#define __hash_mix(a, b, c) do { \
        a -= b; a -= c; a ^= ( c >> 13 ); \
        b -= c; b -= a; b ^= ( a << 8 ); \
        c -= a; c -= b; c ^= ( b >> 13 ); \
        a -= b; a -= c; a ^= ( c >> 12 ); \
        b -= c; b -= a; b ^= ( a << 16 ); \
        c -= a; c -= b; c ^= ( b >> 5 ); \
        a -= b; a -= c; a ^= ( c >> 3 ); \
        b -= c; b -= a; b ^= ( a << 10 ); \
        c -= a; c -= b; c ^= ( b >> 15 ); \
    } while (0)

#define __hash(key, length) ({ \
    unsigned _i, _j, _k; \
    char *_key = (char *)(key); \
    unsigned _hash = 0xfeedbeef; \
    _i = _j = 0x9e3779b9; \
    _k = length; \
    while (_k >= 12) { \
        _i += (_key[0] + ((unsigned)_key[1] << 8) \
            + ((unsigned)_key[2] << 16) \
            + ((unsigned)_key[3] << 24)); \
        _j += (_key[4] + ((unsigned)_key[5] << 8) \
            + ((unsigned)_key[6] << 16) \
            + ((unsigned)_key[7] << 24)); \
        _hash += (_key[8] + ((unsigned)_key[9] << 8) \
            + ((unsigned)_key[10] << 16) \
            + ((unsigned)_key[11] << 24)); \
        __hash_mix(_i, _j, _hash); \
        _key += 12; \
        _k -= 12; \
    } \
    _hash += length; \
    switch (_k) { \
        case 11: _hash += ((unsigned)_key[10] << 24); \
        case 10: _hash += ((unsigned)_key[9] << 16); \
        case 9:  _hash += ((unsigned)_key[8] << 8); \
        case 8:  _j += ((unsigned)_key[7] << 24); \
        case 7:  _j += ((unsigned)_key[6] << 16); \
        case 6:  _j += ((unsigned)_key[5] << 8); \
        case 5:  _j += _key[4]; \
        case 4:  _i += ((unsigned)_key[3] << 24); \
        case 3:  _i += ((unsigned)_key[2] << 16); \
        case 2:  _i += ((unsigned)_key[1] << 8); \
        case 1:  _i += _key[0]; \
    } \
    __hash_mix(_i, _j, _hash); \
    _hash; })

/*
 * Knuth recommends primes in approximately golden ratio to the maximum
 * integer representable by a machine word for multiplicative _hashing. Chuck
 * Lever verified the effectiveness of this technique:
 * http://www.citi.umich.edu/techreports/reports/citi-tr-00-1.pdf
 *
 * These primes are chosen to be bit-sparse, that is operations on them can
 * use shifts and additions instead of multiplications for machines where
 * multiplications are slow.
 */

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

#if __WORDSIZE == 32
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_PRIME_32
#define _hash_long(val, bits) _hash_32(val, bits)
#elif __WORDSIZE == 64
#define _hash_long(val, bits) _hash_64(val, bits)
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_PRIME_64
#endif

static inline uint64_t _hash_64(uint64_t val, unsigned int bits) {
    uint64_t _hash = val;

    uint64_t n = _hash;
    n <<= 18;
    _hash -= n;
    n <<= 33;
    _hash -= n;
    n <<= 3;
    _hash += n;
    n <<= 3;
    _hash -= n;
    n <<= 4;
    _hash += n;
    n <<= 2;
    _hash += n;

    /* high bits are more random, so use them */
    return _hash >> (64 - bits);
}

static inline uint32_t _hash_32(uint32_t val, unsigned int bits) {
    uint32_t _hash = val * GOLDEN_RATIO_PRIME_32;

    /* high bits are more random, so use them */
    return _hash >> (32 - bits);
}

static inline unsigned long _hash_ptr(const void *ptr, unsigned int bits) {
    return _hash_long((unsigned long)ptr, bits);
}

#endif /* !HASH_H_ */
