#ifndef COMPAT_H
#define COMPAT_H

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Replacement for g_malloc0() */
#define g_malloc0(n) calloc(1, (n))

/* Replacement for g_free() */
#define g_free(ptr) free(ptr)

/* Compute the number of elements in a statically allocated array */
#define G_N_ELEMENTS(x) (sizeof(x) / sizeof((x)[0]))

/*
 * GINT64_FROM_LE:
 *
 * Convert a 64-bit integer stored in little-endian order to host order.
 * (On a little-endian machine this is a no-op.)
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define GINT64_FROM_LE(x) (x)
#else
#define GINT64_FROM_LE(x) __builtin_bswap64(x)
#endif

#endif /* COMPAT_H */
