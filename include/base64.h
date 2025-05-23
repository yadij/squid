/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_INCLUDE_BASE64_H
#define SQUID_INCLUDE_BASE64_H

#if HAVE_NETTLE_BASE64_H
#include <nettle/base64.h>

#else /* Base64 functions copied from Nettle 3.4 under GPLv2, with adjustments */

/* base64.h

   Base-64 encoding and decoding.

   Copyright (C) 2002 Niels Möller, Dan Egnor

   This file is part of GNU Nettle.

   GNU Nettle is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.

   GNU Nettle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see http://www.gnu.org/licenses/.
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Base64 encoding */

/* Maximum length of output for base64_encode_update. NOTE: Doesn't
 * include any padding that base64_encode_final may add. */
/* We have at most 4 buffered bits, and a total of (4 + length * 8) bits. */
#define BASE64_ENCODE_LENGTH(length) (((length) * 8 + 4)/6)

/* Maximum length of output generated by base64_encode_final. */
#define BASE64_ENCODE_FINAL_LENGTH 3

/* Exact length of output generated by base64_encode_raw, including
 * padding. */
#define BASE64_ENCODE_RAW_LENGTH(length) ((((length) + 2)/3)*4)

struct base64_encode_ctx
{
    const char *alphabet;    /* Alphabet to use for encoding */
    unsigned short word;     /* Leftover bits */
    unsigned char bits;      /* Number of bits, always 0, 2, or 4. */
};

/* Initialize encoding context for base-64 */
void
base64_encode_init(struct base64_encode_ctx *ctx);

/* Initialize encoding context for URL safe alphabet, RFC 4648. */
void
base64url_encode_init(struct base64_encode_ctx *ctx);

/* Encodes a single byte. Returns amount of output (always 1 or 2). */
size_t
base64_encode_single(struct base64_encode_ctx *ctx,
                     char *dst,
                     uint8_t src);

/* Returns the number of output characters. DST should point to an
 * area of size at least BASE64_ENCODE_LENGTH(length). */
size_t
base64_encode_update(struct base64_encode_ctx *ctx,
                     char *dst,
                     size_t length,
                     const uint8_t *src);

/* DST should point to an area of size at least
 * BASE64_ENCODE_FINAL_LENGTH */
size_t
base64_encode_final(struct base64_encode_ctx *ctx,
                    char *dst);

/* Lower level functions */

/* Encodes a string in one go, including any padding at the end.
 * Generates exactly BASE64_ENCODE_RAW_LENGTH(length) bytes of output.
 * Supports overlapped operation, if src <= dst.
 * TODO: Use of overlap is deprecated, if needed there should be a separate public function
 * to do that.*/
void
base64_encode_raw(char *dst, size_t length, const uint8_t *src);

void
base64_encode_group(char *dst, uint32_t group);

/* Base64 decoding */

/* Maximum length of output for base64_decode_update. */
/* We have at most 6 buffered bits, and a total of (length + 1) * 6 bits. */
#define BASE64_DECODE_LENGTH(length) ((((length) + 1) * 6) / 8)

struct base64_decode_ctx
{
    const signed char *table; /* Decoding table */
    unsigned short word;      /* Leftover bits */
    unsigned char bits;       /* Number buffered bits */

    /* Number of padding characters encountered */
    unsigned char padding;
};

/* Initialize decoding context for base-64 */
void
base64_decode_init(struct base64_decode_ctx *ctx);

/* Initialize encoding context for URL safe alphabet, RFC 4648. */
void
base64url_decode_init(struct base64_decode_ctx *ctx);

/* Decodes a single byte. Returns amount of output (0 or 1), or -1 on
 * errors. */
int
base64_decode_single(struct base64_decode_ctx *ctx,
                     uint8_t *dst,
                     char src);

/* Returns 1 on success, 0 on error. DST should point to an area of
 * size at least BASE64_DECODE_LENGTH(length). The amount of data
 * generated is returned in *DST_LENGTH. */
int
base64_decode_update(struct base64_decode_ctx *ctx,
                     size_t *dst_length,
                     uint8_t *dst,
                     size_t src_length,
                     const char *src);

/* Returns 1 on success. */
int
base64_decode_final(struct base64_decode_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* HAVE_NETTLE_BASE64_H */

/// Calculate the buffer size required to hold the encoded form of
/// a string of length 'decodedLen' including all terminator bytes.
#define base64_encode_len(length) (BASE64_ENCODE_LENGTH(length)+BASE64_ENCODE_FINAL_LENGTH+1)

#endif /* SQUID_INCLUDE_BASE64_H */

