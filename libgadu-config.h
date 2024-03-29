/* Local libgadu configuration. */

#ifndef __GG_LIBGADU_CONFIG_H
#define __GG_LIBGADU_CONFIG_H

/* Defined if libgadu was compiled for bigendian machine. */
//#undef __GG_LIBGADU_BIGENDIAN

/* Defined if libgadu was compiled and linked with pthread support. */
//#undef __GG_LIBGADU_HAVE_PTHREAD

/* Defined if this machine has C99-compiliant vsnprintf(). */
#undef __GG_LIBGADU_HAVE_C99_VSNPRINTF

/* Defined if this machine has va_copy(). */
#undef __GG_LIBGADU_HAVE_VA_COPY

/* Defined if this machine has __va_copy(). */
//#undef __GG_LIBGADU_HAVE___VA_COPY

/* Defined if this machine supports long long. */
//#undef __GG_LIBGADU_HAVE_LONG_LONG

/* Defined if libgadu was compiled and linked with TLS support. */
#define __GG_LIBGADU_HAVE_OPENSSL

/* Include file containing uintXX_t declarations. */
#include <stdint.h>

#endif /* __GG_LIBGADU_CONFIG_H */
