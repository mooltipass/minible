// Monocypher version __git__
//
// SPDX-License-Identifier: BSD-2-Clause
//
// ------------------------------------------------------------------------
//
// Copyright (c) 2017-2020, Loup Vaillant
// All rights reserved.
//
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ------------------------------------------------------------------------

#include "monocypher.h"

/////////////////
/// Utilities ///
/////////////////
#define FOR_T(type, i, start, end) for (type i = (start); i < (end); i++)
#define FOR(i, start, end)         FOR_T(size_t, i, start, end)
#define COPY(dst, src, size)       FOR(i__, 0, size) (dst)[i__] = (src)[i__]
#define ZERO(buf, size)            FOR(i__, 0, size) (buf)[i__] = 0
#define WIPE_CTX(ctx)              crypto_wipe(ctx   , sizeof(*(ctx)))
#define WIPE_BUFFER(buffer)        crypto_wipe(buffer, sizeof(buffer))
#define MIN(a, b)                  ((a) <= (b) ? (a) : (b))
#define MAX(a, b)                  ((a) >= (b) ? (a) : (b))

typedef int8_t   i8;
typedef uint8_t  u8;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint64_t u64;

static u32 load24_le(const u8 s[3])
{
    return (u32)s[0]
        | ((u32)s[1] <<  8)
        | ((u32)s[2] << 16);
}

static u32 load32_le(const u8 s[4])
{
    return (u32)s[0]
        | ((u32)s[1] <<  8)
        | ((u32)s[2] << 16)
        | ((u32)s[3] << 24);
}

static u64 load64_le(const u8 s[8])
{
    return load32_le(s) | ((u64)load32_le(s+4) << 32);
}

static void store32_le(u8 out[4], u32 in)
{
    out[0] =  in        & 0xff;
    out[1] = (in >>  8) & 0xff;
    out[2] = (in >> 16) & 0xff;
    out[3] = (in >> 24) & 0xff;
}

static void load32_le_buf (u32 *dst, const u8 *src, size_t size) {
    FOR(i, 0, size) { dst[i] = load32_le(src + i*4); }
}
static void store32_le_buf(u8 *dst, const u32 *src, size_t size) {
    FOR(i, 0, size) { store32_le(dst + i*4, src[i]); }
}

static int neq0(u64 diff)
{   // constant time comparison to zero
    // return diff != 0 ? -1 : 0
    u64 half = (diff >> 32) | ((u32)diff);
    return (1 & ((half - 1) >> 32)) - 1;
}

static u64 x16(const u8 a[16], const u8 b[16])
{
    return (load64_le(a + 0) ^ load64_le(b + 0))
        |  (load64_le(a + 8) ^ load64_le(b + 8));
}

static u64 x32(const u8 a[32],const u8 b[32]){return x16(a,b)| x16(a+16, b+16);}
int crypto_verify32(const u8 a[32], const u8 b[32]){ return neq0(x32(a, b)); }

void crypto_wipe(void *secret, size_t size)
{
    volatile u8 *v_secret = (u8*)secret;
    ZERO(v_secret, size);
}

////////////////////////////////////
/// Arithmetic modulo 2^255 - 19 ///
////////////////////////////////////
//  Originally taken from SUPERCOP's ref10 implementation.
//  A bit bigger than TweetNaCl, over 4 times faster.

// field element
typedef i32 fe[10];

// field constants
//
// fe_one      : 1
// sqrtm1      : sqrt(-1)
// d           :     -121665 / 121666
// D2          : 2 * -121665 / 121666
// lop_x, lop_y: low order point in Edwards coordinates
// ufactor     : -sqrt(-1) * 2
// A2          : 486662^2  (A squared)
static const fe sqrtm1  = {-32595792, -7943725, 9377950, 3500415, 12389472,
                           -272473, -25146209, -2005654, 326686, 11406482,};
static const fe d       = {-10913610, 13857413, -15372611, 6949391, 114729,
                           -8787816, -6275908, -3247719, -18696448, -12055116,};
static const fe D2      = {-21827239, -5839606, -30745221, 13898782, 229458,
                           15978800, -12551817, -6495438, 29715968, 9444199,};


static void fe_0(fe h) {           ZERO(h  , 10); }
static void fe_1(fe h) { h[0] = 1; ZERO(h+1,  9); }

static void fe_copy(fe h,const fe f           ){FOR(i,0,10) h[i] =  f[i];      }
static void fe_neg (fe h,const fe f           ){FOR(i,0,10) h[i] = -f[i];      }
static void fe_add (fe h,const fe f,const fe g){FOR(i,0,10) h[i] = f[i] + g[i];}
static void fe_sub (fe h,const fe f,const fe g){FOR(i,0,10) h[i] = f[i] - g[i];}

static void fe_cswap(fe f, fe g, int b)
{
    i32 mask = -b; // -1 = 0xffffffff
    FOR (i, 0, 10) {
        i32 x = (f[i] ^ g[i]) & mask;
        f[i] = f[i] ^ x;
        g[i] = g[i] ^ x;
    }
}

static void fe_ccopy(fe f, const fe g, int b)
{
    i32 mask = -b; // -1 = 0xffffffff
    FOR (i, 0, 10) {
        i32 x = (f[i] ^ g[i]) & mask;
        f[i] = f[i] ^ x;
    }
}


// Signed carry propagation
// ------------------------
//
// Let t be a number.  It can be uniquely decomposed thus:
//
//    t = h*2^26 + l
//    such that -2^25 <= l < 2^25
//
// Let c = (t + 2^25) / 2^26            (rounded down)
//     c = (h*2^26 + l + 2^25) / 2^26   (rounded down)
//     c =  h   +   (l + 2^25) / 2^26   (rounded down)
//     c =  h                           (exactly)
// Because 0 <= l + 2^25 < 2^26
//
// Let u = t          - c*2^26
//     u = h*2^26 + l - h*2^26
//     u = l
// Therefore, -2^25 <= u < 2^25
//
// Additionally, if |t| < x, then |h| < x/2^26 (rounded down)
//
// Notations:
// - In C, 1<<25 means 2^25.
// - In C, x>>25 means floor(x / (2^25)).
// - All of the above applies with 25 & 24 as well as 26 & 25.
//
//
// Note on negative right shifts
// -----------------------------
//
// In C, x >> n, where x is a negative integer, is implementation
// defined.  In practice, all platforms do arithmetic shift, which is
// equivalent to division by 2^26, rounded down.  Some compilers, like
// GCC, even guarantee it.
//
// If we ever stumble upon a platform that does not propagate the sign
// bit (we won't), visible failures will show at the slightest test, and
// the signed shifts can be replaced by the following:
//
//     typedef struct { i64 x:39; } s25;
//     typedef struct { i64 x:38; } s26;
//     i64 shift25(i64 x) { s25 s; s.x = ((u64)x)>>25; return s.x; }
//     i64 shift26(i64 x) { s26 s; s.x = ((u64)x)>>26; return s.x; }
//
// Current compilers cannot optimise this, causing a 30% drop in
// performance.  Fairly expensive for something that never happens.
//
//
// Precondition
// ------------
//
// |t0|       < 2^63
// |t1|..|t9| < 2^62
//
// Algorithm
// ---------
// c   = t0 + 2^25 / 2^26   -- |c|  <= 2^36
// t0 -= c * 2^26           -- |t0| <= 2^25
// t1 += c                  -- |t1| <= 2^63
//
// c   = t4 + 2^25 / 2^26   -- |c|  <= 2^36
// t4 -= c * 2^26           -- |t4| <= 2^25
// t5 += c                  -- |t5| <= 2^63
//
// c   = t1 + 2^24 / 2^25   -- |c|  <= 2^38
// t1 -= c * 2^25           -- |t1| <= 2^24
// t2 += c                  -- |t2| <= 2^63
//
// c   = t5 + 2^24 / 2^25   -- |c|  <= 2^38
// t5 -= c * 2^25           -- |t5| <= 2^24
// t6 += c                  -- |t6| <= 2^63
//
// c   = t2 + 2^25 / 2^26   -- |c|  <= 2^37
// t2 -= c * 2^26           -- |t2| <= 2^25        < 1.1 * 2^25  (final t2)
// t3 += c                  -- |t3| <= 2^63
//
// c   = t6 + 2^25 / 2^26   -- |c|  <= 2^37
// t6 -= c * 2^26           -- |t6| <= 2^25        < 1.1 * 2^25  (final t6)
// t7 += c                  -- |t7| <= 2^63
//
// c   = t3 + 2^24 / 2^25   -- |c|  <= 2^38
// t3 -= c * 2^25           -- |t3| <= 2^24        < 1.1 * 2^24  (final t3)
// t4 += c                  -- |t4| <= 2^25 + 2^38 < 2^39
//
// c   = t7 + 2^24 / 2^25   -- |c|  <= 2^38
// t7 -= c * 2^25           -- |t7| <= 2^24        < 1.1 * 2^24  (final t7)
// t8 += c                  -- |t8| <= 2^63
//
// c   = t4 + 2^25 / 2^26   -- |c|  <= 2^13
// t4 -= c * 2^26           -- |t4| <= 2^25        < 1.1 * 2^25  (final t4)
// t5 += c                  -- |t5| <= 2^24 + 2^13 < 1.1 * 2^24  (final t5)
//
// c   = t8 + 2^25 / 2^26   -- |c|  <= 2^37
// t8 -= c * 2^26           -- |t8| <= 2^25        < 1.1 * 2^25  (final t8)
// t9 += c                  -- |t9| <= 2^63
//
// c   = t9 + 2^24 / 2^25   -- |c|  <= 2^38
// t9 -= c * 2^25           -- |t9| <= 2^24        < 1.1 * 2^24  (final t9)
// t0 += c * 19             -- |t0| <= 2^25 + 2^38*19 < 2^44
//
// c   = t0 + 2^25 / 2^26   -- |c|  <= 2^18
// t0 -= c * 2^26           -- |t0| <= 2^25        < 1.1 * 2^25  (final t0)
// t1 += c                  -- |t1| <= 2^24 + 2^18 < 1.1 * 2^24  (final t1)
//
// Postcondition
// -------------
//   |t0|, |t2|, |t4|, |t6|, |t8|  <  1.1 * 2^25
//   |t1|, |t3|, |t5|, |t7|, |t9|  <  1.1 * 2^24
#define FE_CARRY                                                        \
    i64 c;                                                              \
    c = (t0 + ((i64)1<<25)) >> 26;  t0 -= c * ((i64)1 << 26);  t1 += c; \
    c = (t4 + ((i64)1<<25)) >> 26;  t4 -= c * ((i64)1 << 26);  t5 += c; \
    c = (t1 + ((i64)1<<24)) >> 25;  t1 -= c * ((i64)1 << 25);  t2 += c; \
    c = (t5 + ((i64)1<<24)) >> 25;  t5 -= c * ((i64)1 << 25);  t6 += c; \
    c = (t2 + ((i64)1<<25)) >> 26;  t2 -= c * ((i64)1 << 26);  t3 += c; \
    c = (t6 + ((i64)1<<25)) >> 26;  t6 -= c * ((i64)1 << 26);  t7 += c; \
    c = (t3 + ((i64)1<<24)) >> 25;  t3 -= c * ((i64)1 << 25);  t4 += c; \
    c = (t7 + ((i64)1<<24)) >> 25;  t7 -= c * ((i64)1 << 25);  t8 += c; \
    c = (t4 + ((i64)1<<25)) >> 26;  t4 -= c * ((i64)1 << 26);  t5 += c; \
    c = (t8 + ((i64)1<<25)) >> 26;  t8 -= c * ((i64)1 << 26);  t9 += c; \
    c = (t9 + ((i64)1<<24)) >> 25;  t9 -= c * ((i64)1 << 25);  t0 += c * 19; \
    c = (t0 + ((i64)1<<25)) >> 26;  t0 -= c * ((i64)1 << 26);  t1 += c; \
    h[0]=(i32)t0;  h[1]=(i32)t1;  h[2]=(i32)t2;  h[3]=(i32)t3;  h[4]=(i32)t4; \
    h[5]=(i32)t5;  h[6]=(i32)t6;  h[7]=(i32)t7;  h[8]=(i32)t8;  h[9]=(i32)t9

// Decodes a field element from a byte buffer.
// nb_clamps specifies how many bits we ignore.
// Traditionally we ignore 1. It's useful for EdDSA,
// which uses that bit to denote the sign of x.
static void fe_frombytes(fe h, const u8 s[32], unsigned nb_clamp)
{
    i32 clamp = 0xffffff >> nb_clamp;
    i64 t0 =  load32_le(s);                        // t0 < 2^32
    i64 t1 =  load24_le(s +  4) << 6;              // t1 < 2^30
    i64 t2 =  load24_le(s +  7) << 5;              // t2 < 2^29
    i64 t3 =  load24_le(s + 10) << 3;              // t3 < 2^27
    i64 t4 =  load24_le(s + 13) << 2;              // t4 < 2^26
    i64 t5 =  load32_le(s + 16);                   // t5 < 2^32
    i64 t6 =  load24_le(s + 20) << 7;              // t6 < 2^31
    i64 t7 =  load24_le(s + 23) << 5;              // t7 < 2^29
    i64 t8 =  load24_le(s + 26) << 4;              // t8 < 2^28
    i64 t9 = (load24_le(s + 29) & clamp) << 2;     // t9 < 2^25
    FE_CARRY;                                      // Carry precondition OK
}

// Precondition
//   |h[0]|, |h[2]|, |h[4]|, |h[6]|, |h[8]|  <  1.1 * 2^25
//   |h[1]|, |h[3]|, |h[5]|, |h[7]|, |h[9]|  <  1.1 * 2^24
//
// Therefore, |h| < 2^255-19
// There are two possibilities:
//
// - If h is positive, all we need to do is reduce its individual
//   limbs down to their tight positive range.
// - If h is negative, we also need to add 2^255-19 to it.
//   Or just remove 19 and chop off any excess bit.
static void fe_tobytes(u8 s[32], const fe h)
{
    i32 t[10];
    COPY(t, h, 10);
    i32 q = (19 * t[9] + (((i32) 1) << 24)) >> 25;
    //                 |t9|                    < 1.1 * 2^24
    //  -1.1 * 2^24  <  t9                     < 1.1 * 2^24
    //  -21  * 2^24  <  19 * t9                < 21  * 2^24
    //  -2^29        <  19 * t9 + 2^24         < 2^29
    //  -2^29 / 2^25 < (19 * t9 + 2^24) / 2^25 < 2^29 / 2^25
    //  -16          < (19 * t9 + 2^24) / 2^25 < 16
    FOR (i, 0, 5) {
        q += t[2*i  ]; q >>= 26; // q = 0 or -1
        q += t[2*i+1]; q >>= 25; // q = 0 or -1
    }
    // q =  0 iff h >= 0
    // q = -1 iff h <  0
    // Adding q * 19 to h reduces h to its proper range.
    q *= 19;  // Shift carry back to the beginning
    FOR (i, 0, 5) {
        t[i*2  ] += q;  q = t[i*2  ] >> 26;  t[i*2  ] -= q * ((i32)1 << 26);
        t[i*2+1] += q;  q = t[i*2+1] >> 25;  t[i*2+1] -= q * ((i32)1 << 25);
    }
    // h is now fully reduced, and q represents the excess bit.

    store32_le(s +  0, ((u32)t[0] >>  0) | ((u32)t[1] << 26));
    store32_le(s +  4, ((u32)t[1] >>  6) | ((u32)t[2] << 19));
    store32_le(s +  8, ((u32)t[2] >> 13) | ((u32)t[3] << 13));
    store32_le(s + 12, ((u32)t[3] >> 19) | ((u32)t[4] <<  6));
    store32_le(s + 16, ((u32)t[5] >>  0) | ((u32)t[6] << 25));
    store32_le(s + 20, ((u32)t[6] >>  7) | ((u32)t[7] << 19));
    store32_le(s + 24, ((u32)t[7] >> 13) | ((u32)t[8] << 12));
    store32_le(s + 28, ((u32)t[8] >> 20) | ((u32)t[9] <<  6));

    WIPE_BUFFER(t);
}

// Precondition
// -------------
//   |f0|, |f2|, |f4|, |f6|, |f8|  <  1.65 * 2^26
//   |f1|, |f3|, |f5|, |f7|, |f9|  <  1.65 * 2^25
//
//   |g0|, |g2|, |g4|, |g6|, |g8|  <  1.65 * 2^26
//   |g1|, |g3|, |g5|, |g7|, |g9|  <  1.65 * 2^25
static void fe_mul_small(fe h, const fe f, i32 g)
{
    i64 t0 = f[0] * (i64) g;  i64 t1 = f[1] * (i64) g;
    i64 t2 = f[2] * (i64) g;  i64 t3 = f[3] * (i64) g;
    i64 t4 = f[4] * (i64) g;  i64 t5 = f[5] * (i64) g;
    i64 t6 = f[6] * (i64) g;  i64 t7 = f[7] * (i64) g;
    i64 t8 = f[8] * (i64) g;  i64 t9 = f[9] * (i64) g;
    // |t0|, |t2|, |t4|, |t6|, |t8|  <  1.65 * 2^26 * 2^31  < 2^58
    // |t1|, |t3|, |t5|, |t7|, |t9|  <  1.65 * 2^25 * 2^31  < 2^57

    FE_CARRY; // Carry precondition OK
}

// Precondition
// -------------
//   |f0|, |f2|, |f4|, |f6|, |f8|  <  1.65 * 2^26
//   |f1|, |f3|, |f5|, |f7|, |f9|  <  1.65 * 2^25
//
//   |g0|, |g2|, |g4|, |g6|, |g8|  <  1.65 * 2^26
//   |g1|, |g3|, |g5|, |g7|, |g9|  <  1.65 * 2^25
static void fe_mul(fe h, const fe f, const fe g)
{
    // Everything is unrolled and put in temporary variables.
    // We could roll the loop, but that would make curve25519 twice as slow.
    i32 f0 = f[0]; i32 f1 = f[1]; i32 f2 = f[2]; i32 f3 = f[3]; i32 f4 = f[4];
    i32 f5 = f[5]; i32 f6 = f[6]; i32 f7 = f[7]; i32 f8 = f[8]; i32 f9 = f[9];
    i32 g0 = g[0]; i32 g1 = g[1]; i32 g2 = g[2]; i32 g3 = g[3]; i32 g4 = g[4];
    i32 g5 = g[5]; i32 g6 = g[6]; i32 g7 = g[7]; i32 g8 = g[8]; i32 g9 = g[9];
    i32 F1 = f1*2; i32 F3 = f3*2; i32 F5 = f5*2; i32 F7 = f7*2; i32 F9 = f9*2;
    i32 G1 = g1*19;  i32 G2 = g2*19;  i32 G3 = g3*19;
    i32 G4 = g4*19;  i32 G5 = g5*19;  i32 G6 = g6*19;
    i32 G7 = g7*19;  i32 G8 = g8*19;  i32 G9 = g9*19;
    // |F1|, |F3|, |F5|, |F7|, |F9|  <  1.65 * 2^26
    // |G0|, |G2|, |G4|, |G6|, |G8|  <  2^31
    // |G1|, |G3|, |G5|, |G7|, |G9|  <  2^30

    i64 t0 = f0*(i64)g0 + F1*(i64)G9 + f2*(i64)G8 + F3*(i64)G7 + f4*(i64)G6
        +    F5*(i64)G5 + f6*(i64)G4 + F7*(i64)G3 + f8*(i64)G2 + F9*(i64)G1;
    i64 t1 = f0*(i64)g1 + f1*(i64)g0 + f2*(i64)G9 + f3*(i64)G8 + f4*(i64)G7
        +    f5*(i64)G6 + f6*(i64)G5 + f7*(i64)G4 + f8*(i64)G3 + f9*(i64)G2;
    i64 t2 = f0*(i64)g2 + F1*(i64)g1 + f2*(i64)g0 + F3*(i64)G9 + f4*(i64)G8
        +    F5*(i64)G7 + f6*(i64)G6 + F7*(i64)G5 + f8*(i64)G4 + F9*(i64)G3;
    i64 t3 = f0*(i64)g3 + f1*(i64)g2 + f2*(i64)g1 + f3*(i64)g0 + f4*(i64)G9
        +    f5*(i64)G8 + f6*(i64)G7 + f7*(i64)G6 + f8*(i64)G5 + f9*(i64)G4;
    i64 t4 = f0*(i64)g4 + F1*(i64)g3 + f2*(i64)g2 + F3*(i64)g1 + f4*(i64)g0
        +    F5*(i64)G9 + f6*(i64)G8 + F7*(i64)G7 + f8*(i64)G6 + F9*(i64)G5;
    i64 t5 = f0*(i64)g5 + f1*(i64)g4 + f2*(i64)g3 + f3*(i64)g2 + f4*(i64)g1
        +    f5*(i64)g0 + f6*(i64)G9 + f7*(i64)G8 + f8*(i64)G7 + f9*(i64)G6;
    i64 t6 = f0*(i64)g6 + F1*(i64)g5 + f2*(i64)g4 + F3*(i64)g3 + f4*(i64)g2
        +    F5*(i64)g1 + f6*(i64)g0 + F7*(i64)G9 + f8*(i64)G8 + F9*(i64)G7;
    i64 t7 = f0*(i64)g7 + f1*(i64)g6 + f2*(i64)g5 + f3*(i64)g4 + f4*(i64)g3
        +    f5*(i64)g2 + f6*(i64)g1 + f7*(i64)g0 + f8*(i64)G9 + f9*(i64)G8;
    i64 t8 = f0*(i64)g8 + F1*(i64)g7 + f2*(i64)g6 + F3*(i64)g5 + f4*(i64)g4
        +    F5*(i64)g3 + f6*(i64)g2 + F7*(i64)g1 + f8*(i64)g0 + F9*(i64)G9;
    i64 t9 = f0*(i64)g9 + f1*(i64)g8 + f2*(i64)g7 + f3*(i64)g6 + f4*(i64)g5
        +    f5*(i64)g4 + f6*(i64)g3 + f7*(i64)g2 + f8*(i64)g1 + f9*(i64)g0;
    // t0 < 0.67 * 2^61
    // t1 < 0.41 * 2^61
    // t2 < 0.52 * 2^61
    // t3 < 0.32 * 2^61
    // t4 < 0.38 * 2^61
    // t5 < 0.22 * 2^61
    // t6 < 0.23 * 2^61
    // t7 < 0.13 * 2^61
    // t8 < 0.09 * 2^61
    // t9 < 0.03 * 2^61

    FE_CARRY; // Everything below 2^62, Carry precondition OK
}

// Precondition
// -------------
//   |f0|, |f2|, |f4|, |f6|, |f8|  <  1.65 * 2^26
//   |f1|, |f3|, |f5|, |f7|, |f9|  <  1.65 * 2^25
//
// Note: we could use fe_mul() for this, but this is significantly faster
static void fe_sq(fe h, const fe f)
{
    i32 f0 = f[0]; i32 f1 = f[1]; i32 f2 = f[2]; i32 f3 = f[3]; i32 f4 = f[4];
    i32 f5 = f[5]; i32 f6 = f[6]; i32 f7 = f[7]; i32 f8 = f[8]; i32 f9 = f[9];
    i32 f0_2  = f0*2;   i32 f1_2  = f1*2;   i32 f2_2  = f2*2;   i32 f3_2 = f3*2;
    i32 f4_2  = f4*2;   i32 f5_2  = f5*2;   i32 f6_2  = f6*2;   i32 f7_2 = f7*2;
    i32 f5_38 = f5*38;  i32 f6_19 = f6*19;  i32 f7_38 = f7*38;
    i32 f8_19 = f8*19;  i32 f9_38 = f9*38;
    // |f0_2| , |f2_2| , |f4_2| , |f6_2| , |f8_2|  <  1.65 * 2^27
    // |f1_2| , |f3_2| , |f5_2| , |f7_2| , |f9_2|  <  1.65 * 2^26
    // |f5_38|, |f6_19|, |f7_38|, |f8_19|, |f9_38| <  2^31

    i64 t0 = f0  *(i64)f0    + f1_2*(i64)f9_38 + f2_2*(i64)f8_19
        +    f3_2*(i64)f7_38 + f4_2*(i64)f6_19 + f5  *(i64)f5_38;
    i64 t1 = f0_2*(i64)f1    + f2  *(i64)f9_38 + f3_2*(i64)f8_19
        +    f4  *(i64)f7_38 + f5_2*(i64)f6_19;
    i64 t2 = f0_2*(i64)f2    + f1_2*(i64)f1    + f3_2*(i64)f9_38
        +    f4_2*(i64)f8_19 + f5_2*(i64)f7_38 + f6  *(i64)f6_19;
    i64 t3 = f0_2*(i64)f3    + f1_2*(i64)f2    + f4  *(i64)f9_38
        +    f5_2*(i64)f8_19 + f6  *(i64)f7_38;
    i64 t4 = f0_2*(i64)f4    + f1_2*(i64)f3_2  + f2  *(i64)f2
        +    f5_2*(i64)f9_38 + f6_2*(i64)f8_19 + f7  *(i64)f7_38;
    i64 t5 = f0_2*(i64)f5    + f1_2*(i64)f4    + f2_2*(i64)f3
        +    f6  *(i64)f9_38 + f7_2*(i64)f8_19;
    i64 t6 = f0_2*(i64)f6    + f1_2*(i64)f5_2  + f2_2*(i64)f4
        +    f3_2*(i64)f3    + f7_2*(i64)f9_38 + f8  *(i64)f8_19;
    i64 t7 = f0_2*(i64)f7    + f1_2*(i64)f6    + f2_2*(i64)f5
        +    f3_2*(i64)f4    + f8  *(i64)f9_38;
    i64 t8 = f0_2*(i64)f8    + f1_2*(i64)f7_2  + f2_2*(i64)f6
        +    f3_2*(i64)f5_2  + f4  *(i64)f4    + f9  *(i64)f9_38;
    i64 t9 = f0_2*(i64)f9    + f1_2*(i64)f8    + f2_2*(i64)f7
        +    f3_2*(i64)f6    + f4  *(i64)f5_2;
    // t0 < 0.67 * 2^61
    // t1 < 0.41 * 2^61
    // t2 < 0.52 * 2^61
    // t3 < 0.32 * 2^61
    // t4 < 0.38 * 2^61
    // t5 < 0.22 * 2^61
    // t6 < 0.23 * 2^61
    // t7 < 0.13 * 2^61
    // t8 < 0.09 * 2^61
    // t9 < 0.03 * 2^61

    FE_CARRY;
}

//  Parity check.  Returns 0 if even, 1 if odd
static int fe_isodd(const fe f)
{
    u8 s[32];
    fe_tobytes(s, f);
    u8 isodd = s[0] & 1;
    WIPE_BUFFER(s);
    return isodd;
}

// Returns 1 if equal, 0 if not equal
static int fe_isequal(const fe f, const fe g)
{
    u8 fs[32];
    u8 gs[32];
    fe_tobytes(fs, f);
    fe_tobytes(gs, g);
    int isdifferent = crypto_verify32(fs, gs);
    WIPE_BUFFER(fs);
    WIPE_BUFFER(gs);
    return 1 + isdifferent;
}

// Inverse square root.
// Returns true if x is a non zero square, false otherwise.
// After the call:
//   isr = sqrt(1/x)        if x is a non-zero square.
//   isr = sqrt(sqrt(-1)/x) if x is not a square.
//   isr = 0                if x is zero.
// We do not guarantee the sign of the square root.
//
// Notes:
// Let quartic = x^((p-1)/4)
//
// x^((p-1)/2) = chi(x)
// quartic^2   = chi(x)
// quartic     = sqrt(chi(x))
// quartic     = 1 or -1 or sqrt(-1) or -sqrt(-1)
//
// Note that x is a square if quartic is 1 or -1
// There are 4 cases to consider:
//
// if   quartic         = 1  (x is a square)
// then x^((p-1)/4)     = 1
//      x^((p-5)/4) * x = 1
//      x^((p-5)/4)     = 1/x
//      x^((p-5)/8)     = sqrt(1/x) or -sqrt(1/x)
//
// if   quartic                = -1  (x is a square)
// then x^((p-1)/4)            = -1
//      x^((p-5)/4) * x        = -1
//      x^((p-5)/4)            = -1/x
//      x^((p-5)/8)            = sqrt(-1)   / sqrt(x)
//      x^((p-5)/8) * sqrt(-1) = sqrt(-1)^2 / sqrt(x)
//      x^((p-5)/8) * sqrt(-1) = -1/sqrt(x)
//      x^((p-5)/8) * sqrt(-1) = -sqrt(1/x) or sqrt(1/x)
//
// if   quartic         = sqrt(-1)  (x is not a square)
// then x^((p-1)/4)     = sqrt(-1)
//      x^((p-5)/4) * x = sqrt(-1)
//      x^((p-5)/4)     = sqrt(-1)/x
//      x^((p-5)/8)     = sqrt(sqrt(-1)/x) or -sqrt(sqrt(-1)/x)
//
// Note that the product of two non-squares is always a square:
//   For any non-squares a and b, chi(a) = -1 and chi(b) = -1.
//   Since chi(x) = x^((p-1)/2), chi(a)*chi(b) = chi(a*b) = 1.
//   Therefore a*b is a square.
//
//   Since sqrt(-1) and x are both non-squares, their product is a
//   square, and we can compute their square root.
//
// if   quartic                = -sqrt(-1)  (x is not a square)
// then x^((p-1)/4)            = -sqrt(-1)
//      x^((p-5)/4) * x        = -sqrt(-1)
//      x^((p-5)/4)            = -sqrt(-1)/x
//      x^((p-5)/8)            = sqrt(-sqrt(-1)/x)
//      x^((p-5)/8)            = sqrt( sqrt(-1)/x) * sqrt(-1)
//      x^((p-5)/8) * sqrt(-1) = sqrt( sqrt(-1)/x) * sqrt(-1)^2
//      x^((p-5)/8) * sqrt(-1) = sqrt( sqrt(-1)/x) * -1
//      x^((p-5)/8) * sqrt(-1) = -sqrt(sqrt(-1)/x) or sqrt(sqrt(-1)/x)
static int invsqrt(fe isr, const fe x)
{
    fe t0, t1, t2;

    // t0 = x^((p-5)/8)
    // Can be achieved with a simple double & add ladder,
    // but it would be slower.
    fe_sq(t0, x);
    fe_sq(t1,t0);                   fe_sq(t1, t1);  fe_mul(t1, x, t1);
    fe_mul(t0, t0, t1);
    fe_sq(t0, t0);                                  fe_mul(t0, t1, t0);
    fe_sq(t1, t0);  FOR (i, 1,   5) fe_sq(t1, t1);  fe_mul(t0, t1, t0);
    fe_sq(t1, t0);  FOR (i, 1,  10) fe_sq(t1, t1);  fe_mul(t1, t1, t0);
    fe_sq(t2, t1);  FOR (i, 1,  20) fe_sq(t2, t2);  fe_mul(t1, t2, t1);
    fe_sq(t1, t1);  FOR (i, 1,  10) fe_sq(t1, t1);  fe_mul(t0, t1, t0);
    fe_sq(t1, t0);  FOR (i, 1,  50) fe_sq(t1, t1);  fe_mul(t1, t1, t0);
    fe_sq(t2, t1);  FOR (i, 1, 100) fe_sq(t2, t2);  fe_mul(t1, t2, t1);
    fe_sq(t1, t1);  FOR (i, 1,  50) fe_sq(t1, t1);  fe_mul(t0, t1, t0);
    fe_sq(t0, t0);  FOR (i, 1,   2) fe_sq(t0, t0);  fe_mul(t0, t0, x);

    // quartic = x^((p-1)/4)
    i32 *quartic = t1;
    fe_sq (quartic, t0);
    fe_mul(quartic, quartic, x);

    i32 *check = t2;
    fe_1  (check);          int p1 = fe_isequal(quartic, check);
    fe_neg(check, check );  int m1 = fe_isequal(quartic, check);
    fe_neg(check, sqrtm1);  int ms = fe_isequal(quartic, check);

    // if quartic == -1 or sqrt(-1)
    // then  isr = x^((p-1)/4) * sqrt(-1)
    // else  isr = x^((p-1)/4)
    fe_mul(isr, t0, sqrtm1);
    fe_ccopy(isr, t0, 1 - (m1 | ms));

    WIPE_BUFFER(t0);
    WIPE_BUFFER(t1);
    WIPE_BUFFER(t2);
    return p1 | m1;
}

// Inverse in terms of inverse square root.
// Requires two additional squarings to get rid of the sign.
//
//   1/x = x * (+invsqrt(x^2))^2
//       = x * (-invsqrt(x^2))^2
//
// A fully optimised exponentiation by p-1 would save 6 field
// multiplications, but it would require more code.
static void fe_invert(fe out, const fe x)
{
    fe tmp;
    fe_sq(tmp, x);
    invsqrt(tmp, tmp);
    fe_sq(tmp, tmp);
    fe_mul(out, tmp, x);
    WIPE_BUFFER(tmp);
}

// trim a scalar for scalar multiplication
static void trim_scalar(u8 scalar[32])
{
    scalar[ 0] &= 248;
    scalar[31] &= 127;
    scalar[31] |= 64;
}

// get bit from scalar at position i
static int scalar_bit(const u8 s[32], int i)
{
    if (i < 0) { return 0; } // handle -1 for sliding windows
    return (s[i>>3] >> (i&7)) & 1;
}

///////////////////////////
/// Arithmetic modulo L ///
///////////////////////////
static const u32 L[8] = {0x5cf5d3ed, 0x5812631a, 0xa2f79cd6, 0x14def9de,
                         0x00000000, 0x00000000, 0x00000000, 0x10000000,};

//  p = a*b + p
static void multiply(u32 p[16], const u32 a[8], const u32 b[8])
{
    FOR (i, 0, 8) {
        u64 carry = 0;
        FOR (j, 0, 8) {
            carry  += p[i+j] + (u64)a[i] * b[j];
            p[i+j]  = (u32)carry;
            carry >>= 32;
        }
        p[i+8] = (u32)carry;
    }
}

static int is_above_l(const u32 x[8])
{
    // We work with L directly, in a 2's complement encoding
    // (-L == ~L + 1)
    u64 carry = 1;
    FOR (i, 0, 8) {
        carry  += (u64)x[i] + (~L[i] & 0xffffffff);
        carry >>= 32;
    }
    return (int)carry; // carry is either 0 or 1
}

// Final reduction modulo L, by conditionally removing L.
// if x < l     , then r = x
// if l <= x 2*l, then r = x-l
// otherwise the result will be wrong
static void remove_l(u32 r[8], const u32 x[8])
{
    u64 carry = is_above_l(x);
    u32 mask  = ~(u32)carry + 1; // carry == 0 or 1
    FOR (i, 0, 8) {
        carry += (u64)x[i] + (~L[i] & mask);
        r[i]   = (u32)carry;
        carry >>= 32;
    }
}

// Full reduction modulo L (Barrett reduction)
static void mod_l(u8 reduced[32], const u32 x[16])
{
    static const u32 r[9] = {0x0a2c131b,0xed9ce5a3,0x086329a7,0x2106215d,
                             0xffffffeb,0xffffffff,0xffffffff,0xffffffff,0xf,};
    // xr = x * r
    u32 xr[25] = {0};
    FOR (i, 0, 9) {
        u64 carry = 0;
        FOR (j, 0, 16) {
            carry  += xr[i+j] + (u64)r[i] * x[j];
            xr[i+j] = (u32)carry;
            carry >>= 32;
        }
        xr[i+16] = (u32)carry;
    }
    // xr = floor(xr / 2^512) * L
    // Since the result is guaranteed to be below 2*L,
    // it is enough to only compute the first 256 bits.
    // The division is performed by saying xr[i+16]. (16 * 32 = 512)
    ZERO(xr, 8);
    FOR (i, 0, 8) {
        u64 carry = 0;
        FOR (j, 0, 8-i) {
            carry   += xr[i+j] + (u64)xr[i+16] * L[j];
            xr[i+j] = (u32)carry;
            carry >>= 32;
        }
    }
    // xr = x - xr
    u64 carry = 1;
    FOR (i, 0, 8) {
        carry  += (u64)x[i] + (~xr[i] & 0xffffffff);
        xr[i]   = (u32)carry;
        carry >>= 32;
    }
    // Final reduction modulo L (conditional subtraction)
    remove_l(xr, xr);
    store32_le_buf(reduced, xr, 8);

    WIPE_BUFFER(xr);
}

static void reduce(u8 r[64])
{
    u32 x[16];
    load32_le_buf(x, r, 16);
    mod_l(r, x);
    WIPE_BUFFER(x);
}

// r = (a * b) + c
static void mul_add(u8 r[32], const u8 a[32], const u8 b[32], const u8 c[32])
{
    u32 A[8];  load32_le_buf(A, a, 8);
    u32 B[8];  load32_le_buf(B, b, 8);
    u32 p[16];
    load32_le_buf(p, c, 8);
    ZERO(p + 8, 8);
    multiply(p, A, B);
    mod_l(r, p);
    WIPE_BUFFER(p);
    WIPE_BUFFER(A);
    WIPE_BUFFER(B);
}

///////////////
/// Ed25519 ///
///////////////

// Point (group element, ge) in a twisted Edwards curve,
// in extended projective coordinates.
// ge        : x  = X/Z, y  = Y/Z, T  = XY/Z
// ge_cached : Yp = X+Y, Ym = X-Y, T2 = T*D2
// ge_precomp: Z  = 1
typedef struct { fe X;  fe Y;  fe Z; fe T;  } ge;
typedef struct { fe Yp; fe Ym; fe Z; fe T2; } ge_cached;
typedef struct { fe Yp; fe Ym;       fe T2; } ge_precomp;

static void ge_zero(ge *p)
{
    fe_0(p->X);
    fe_1(p->Y);
    fe_1(p->Z);
    fe_0(p->T);
}

static void ge_tobytes(u8 s[32], const ge *h)
{
    fe recip, x, y;
    fe_invert(recip, h->Z);
    fe_mul(x, h->X, recip);
    fe_mul(y, h->Y, recip);
    fe_tobytes(s, y);
    s[31] ^= fe_isodd(x) << 7;

    WIPE_BUFFER(recip);
    WIPE_BUFFER(x);
    WIPE_BUFFER(y);
}

// h = s, where s is a point encoded in 32 bytes
//
// Variable time!  Inputs must not be secret!
// => Use only to *check* signatures.
//
// From the specifications:
//   The encoding of s contains y and the sign of x
//   x = sqrt((y^2 - 1) / (d*y^2 + 1))
// In extended coordinates:
//   X = x, Y = y, Z = 1, T = x*y
//
//    Note that num * den is a square iff num / den is a square
//    If num * den is not a square, the point was not on the curve.
// From the above:
//   Let num =   y^2 - 1
//   Let den = d*y^2 + 1
//   x = sqrt((y^2 - 1) / (d*y^2 + 1))
//   x = sqrt(num / den)
//   x = sqrt(num^2 / (num * den))
//   x = num * sqrt(1 / (num * den))
//
// Therefore, we can just compute:
//   num =   y^2 - 1
//   den = d*y^2 + 1
//   isr = invsqrt(num * den)  // abort if not square
//   x   = num * isr
// Finally, negate x if its sign is not as specified.
static int ge_frombytes_vartime(ge *h, const u8 s[32])
{
    fe_frombytes(h->Y, s, 1);
    fe_1(h->Z);
    fe_sq (h->T, h->Y);        // t =   y^2
    fe_mul(h->X, h->T, d   );  // x = d*y^2
    fe_sub(h->T, h->T, h->Z);  // t =   y^2 - 1
    fe_add(h->X, h->X, h->Z);  // x = d*y^2 + 1
    fe_mul(h->X, h->T, h->X);  // x = (y^2 - 1) * (d*y^2 + 1)
    int is_square = invsqrt(h->X, h->X);
    if (!is_square) {
        return -1;             // Not on the curve, abort
    }
    fe_mul(h->X, h->T, h->X);  // x = sqrt((y^2 - 1) / (d*y^2 + 1))
    if (fe_isodd(h->X) != (s[31] >> 7)) {
        fe_neg(h->X, h->X);
    }
    fe_mul(h->T, h->X, h->Y);
    return 0;
}

static void ge_cache(ge_cached *c, const ge *p)
{
    fe_add (c->Yp, p->Y, p->X);
    fe_sub (c->Ym, p->Y, p->X);
    fe_copy(c->Z , p->Z      );
    fe_mul (c->T2, p->T, D2  );
}

// Internal buffers are not wiped! Inputs must not be secret!
// => Use only to *check* signatures.
static void ge_add(ge *s, const ge *p, const ge_cached *q)
{
    fe a, b;
    fe_add(a   , p->Y, p->X );
    fe_sub(b   , p->Y, p->X );
    fe_mul(a   , a   , q->Yp);
    fe_mul(b   , b   , q->Ym);
    fe_add(s->Y, a   , b    );
    fe_sub(s->X, a   , b    );

    fe_add(s->Z, p->Z, p->Z );
    fe_mul(s->Z, s->Z, q->Z );
    fe_mul(s->T, p->T, q->T2);
    fe_add(a   , s->Z, s->T );
    fe_sub(b   , s->Z, s->T );

    fe_mul(s->T, s->X, s->Y);
    fe_mul(s->X, s->X, b   );
    fe_mul(s->Y, s->Y, a   );
    fe_mul(s->Z, a   , b   );
}

// Internal buffers are not wiped! Inputs must not be secret!
// => Use only to *check* signatures.
static void ge_sub(ge *s, const ge *p, const ge_cached *q)
{
    ge_cached neg;
    fe_copy(neg.Ym, q->Yp);
    fe_copy(neg.Yp, q->Ym);
    fe_copy(neg.Z , q->Z );
    fe_neg (neg.T2, q->T2);
    ge_add(s, p, &neg);
}

static void ge_madd(ge *s, const ge *p, const ge_precomp *q, fe a, fe b)
{
    fe_add(a   , p->Y, p->X );
    fe_sub(b   , p->Y, p->X );
    fe_mul(a   , a   , q->Yp);
    fe_mul(b   , b   , q->Ym);
    fe_add(s->Y, a   , b    );
    fe_sub(s->X, a   , b    );

    fe_add(s->Z, p->Z, p->Z );
    fe_mul(s->T, p->T, q->T2);
    fe_add(a   , s->Z, s->T );
    fe_sub(b   , s->Z, s->T );

    fe_mul(s->T, s->X, s->Y);
    fe_mul(s->X, s->X, b   );
    fe_mul(s->Y, s->Y, a   );
    fe_mul(s->Z, a   , b   );
}

// Internal buffers are not wiped! Inputs must not be secret!
// => Use only to *check* signatures.
static void ge_msub(ge *s, const ge *p, const ge_precomp *q, fe a, fe b)
{
    ge_precomp neg;
    fe_copy(neg.Ym, q->Yp);
    fe_copy(neg.Yp, q->Ym);
    fe_neg (neg.T2, q->T2);
    ge_madd(s, p, &neg, a, b);
}

static void ge_double(ge *s, const ge *p, ge *q)
{
    fe_sq (q->X, p->X);
    fe_sq (q->Y, p->Y);
    fe_sq (q->Z, p->Z);          // qZ = pZ^2
    fe_mul_small(q->Z, q->Z, 2); // qZ = pZ^2 * 2
    fe_add(q->T, p->X, p->Y);
    fe_sq (s->T, q->T);
    fe_add(q->T, q->Y, q->X);
    fe_sub(q->Y, q->Y, q->X);
    fe_sub(q->X, s->T, q->T);
    fe_sub(q->Z, q->Z, q->Y);

    fe_mul(s->X, q->X , q->Z);
    fe_mul(s->Y, q->T , q->Y);
    fe_mul(s->Z, q->Y , q->Z);
    fe_mul(s->T, q->X , q->T);
}

// 5-bit signed window in cached format (Niels coordinates, Z=1)
static const ge_precomp b_window[8] = {
    {{25967493,-14356035,29566456,3660896,-12694345,
      4014787,27544626,-11754271,-6079156,2047605,},
     {-12545711,934262,-2722910,3049990,-727428,
      9406986,12720692,5043384,19500929,-15469378,},
     {-8738181,4489570,9688441,-14785194,10184609,
      -12363380,29287919,11864899,-24514362,-4438546,},},
    {{15636291,-9688557,24204773,-7912398,616977,
      -16685262,27787600,-14772189,28944400,-1550024,},
     {16568933,4717097,-11556148,-1102322,15682896,
      -11807043,16354577,-11775962,7689662,11199574,},
     {30464156,-5976125,-11779434,-15670865,23220365,
      15915852,7512774,10017326,-17749093,-9920357,},},
    {{10861363,11473154,27284546,1981175,-30064349,
      12577861,32867885,14515107,-15438304,10819380,},
     {4708026,6336745,20377586,9066809,-11272109,
      6594696,-25653668,12483688,-12668491,5581306,},
     {19563160,16186464,-29386857,4097519,10237984,
      -4348115,28542350,13850243,-23678021,-15815942,},},
    {{5153746,9909285,1723747,-2777874,30523605,
      5516873,19480852,5230134,-23952439,-15175766,},
     {-30269007,-3463509,7665486,10083793,28475525,
      1649722,20654025,16520125,30598449,7715701,},
     {28881845,14381568,9657904,3680757,-20181635,
      7843316,-31400660,1370708,29794553,-1409300,},},
    {{-22518993,-6692182,14201702,-8745502,-23510406,
      8844726,18474211,-1361450,-13062696,13821877,},
     {-6455177,-7839871,3374702,-4740862,-27098617,
      -10571707,31655028,-7212327,18853322,-14220951,},
     {4566830,-12963868,-28974889,-12240689,-7602672,
      -2830569,-8514358,-10431137,2207753,-3209784,},},
    {{-25154831,-4185821,29681144,7868801,-6854661,
      -9423865,-12437364,-663000,-31111463,-16132436,},
     {25576264,-2703214,7349804,-11814844,16472782,
      9300885,3844789,15725684,171356,6466918,},
     {23103977,13316479,9739013,-16149481,817875,
      -15038942,8965339,-14088058,-30714912,16193877,},},
    {{-33521811,3180713,-2394130,14003687,-16903474,
      -16270840,17238398,4729455,-18074513,9256800,},
     {-25182317,-4174131,32336398,5036987,-21236817,
      11360617,22616405,9761698,-19827198,630305,},
     {-13720693,2639453,-24237460,-7406481,9494427,
      -5774029,-6554551,-15960994,-2449256,-14291300,},},
    {{-3151181,-5046075,9282714,6866145,-31907062,
      -863023,-18940575,15033784,25105118,-7894876,},
     {-24326370,15950226,-31801215,-14592823,-11662737,
      -5090925,1573892,-2625887,2198790,-15804619,},
     {-3099351,10324967,-2241613,7453183,-5446979,
      -2735503,-13812022,-16236442,-32461234,-12290683,},},
};

// Incremental sliding windows (left to right)
// Based on Roberto Maria Avanzi[2005]
typedef struct {
    i16 next_index; // position of the next signed digit
    i8  next_digit; // next signed digit (odd number below 2^window_width)
    u8  next_check; // point at which we must check for a new window
} slide_ctx;

static void slide_init(slide_ctx *ctx, const u8 scalar[32])
{
    // scalar is guaranteed to be below L, either because we checked (s),
    // or because we reduced it modulo L (h_ram). L is under 2^253, so
    // so bits 253 to 255 are guaranteed to be zero. No need to test them.
    //
    // Note however that L is very close to 2^252, so bit 252 is almost
    // always zero.  If we were to start at bit 251, the tests wouldn't
    // catch the off-by-one error (constructing one that does would be
    // prohibitively expensive).
    //
    // We should still check bit 252, though.
    int i = 252;
    while (i > 0 && scalar_bit(scalar, i) == 0) {
        i--;
    }
    ctx->next_check = (u8)(i + 1);
    ctx->next_index = -1;
    ctx->next_digit = -1;
}

static int slide_step(slide_ctx *ctx, int width, int i, const u8 scalar[32])
{
    if (i == ctx->next_check) {
        if (scalar_bit(scalar, i) == scalar_bit(scalar, i - 1)) {
            ctx->next_check--;
        } else {
            // compute digit of next window
            int w = MIN(width, i + 1);
            int v = -(scalar_bit(scalar, i) << (w-1));
            FOR_T (int, j, 0, w-1) {
                v += scalar_bit(scalar, i-(w-1)+j) << j;
            }
            v += scalar_bit(scalar, i-w);
            int lsb = v & (~v + 1);            // smallest bit of v
            int s   = (   ((lsb & 0xAA) != 0)  // log2(lsb)
                       | (((lsb & 0xCC) != 0) << 1)
                       | (((lsb & 0xF0) != 0) << 2));
            ctx->next_index  = (i16)(i-(w-1)+s);
            ctx->next_digit  = (i8) (v >> s   );
            ctx->next_check -= (u8) w;
        }
    }
    return i == ctx->next_index ? ctx->next_digit: 0;
}

#define P_W_WIDTH 3 // Affects the size of the stack
#define B_W_WIDTH 5 // Affects the size of the binary
#define P_W_SIZE  (1<<(P_W_WIDTH-2))

// P = [b]B + [p]P, where B is the base point
//
// Variable time! Internal buffers are not wiped! Inputs must not be secret!
// => Use only to *check* signatures.
static void ge_double_scalarmult_vartime(ge *P, const u8 p[32], const u8 b[32])
{
    // cache P window for addition
    ge_cached cP[P_W_SIZE];
    {
        ge P2, tmp;
        ge_double(&P2, P, &tmp);
        ge_cache(&cP[0], P);
        FOR (i, 1, P_W_SIZE) {
            ge_add(&tmp, &P2, &cP[i-1]);
            ge_cache(&cP[i], &tmp);
        }
    }

    // Merged double and add ladder, fused with sliding
    slide_ctx p_slide;  slide_init(&p_slide, p);
    slide_ctx b_slide;  slide_init(&b_slide, b);
    int i = MAX(p_slide.next_check, b_slide.next_check);
    ge *sum = P;
    ge_zero(sum);
    while (i >= 0) {
        ge tmp;
        ge_double(sum, sum, &tmp);
        int p_digit = slide_step(&p_slide, P_W_WIDTH, i, p);
        int b_digit = slide_step(&b_slide, B_W_WIDTH, i, b);
        if (p_digit > 0) { ge_add(sum, sum, &cP[ p_digit / 2]); }
        if (p_digit < 0) { ge_sub(sum, sum, &cP[-p_digit / 2]); }
        fe t1, t2;
        if (b_digit > 0) { ge_madd(sum, sum, b_window +  b_digit/2, t1, t2); }
        if (b_digit < 0) { ge_msub(sum, sum, b_window + -b_digit/2, t1, t2); }
        i--;
    }
}

// R_check = s[B] - h_ram[pk], where B is the base point
//
// Variable time! Internal buffers are not wiped! Inputs must not be secret!
// => Use only to *check* signatures.
static int ge_r_check(u8 R_check[32], u8 s[32], u8 h_ram[32], u8 pk[32])
{
    ge  A;      // not secret, not wiped
    u32 s32[8]; // not secret, not wiped
    load32_le_buf(s32, s, 8);
    if (ge_frombytes_vartime(&A, pk) ||         // A = pk
        is_above_l(s32)) {                      // prevent s malleability
        return -1;
    }
    fe_neg(A.X, A.X);
    fe_neg(A.T, A.T);                           // A = -pk
    ge_double_scalarmult_vartime(&A, h_ram, s); // A = [s]B - [h_ram]pk
    ge_tobytes(R_check, &A);                    // R_check = A
    return 0;
}

// 5-bit signed comb in cached format (Niels coordinates, Z=1)
static const ge_precomp b_comb_low[8] = {
    {{-6816601,-2324159,-22559413,124364,18015490,
      8373481,19993724,1979872,-18549925,9085059,},
     {10306321,403248,14839893,9633706,8463310,
      -8354981,-14305673,14668847,26301366,2818560,},
     {-22701500,-3210264,-13831292,-2927732,-16326337,
      -14016360,12940910,177905,12165515,-2397893,},},
    {{-12282262,-7022066,9920413,-3064358,-32147467,
      2927790,22392436,-14852487,2719975,16402117,},
     {-7236961,-4729776,2685954,-6525055,-24242706,
      -15940211,-6238521,14082855,10047669,12228189,},
     {-30495588,-12893761,-11161261,3539405,-11502464,
      16491580,-27286798,-15030530,-7272871,-15934455,},},
    {{17650926,582297,-860412,-187745,-12072900,
      -10683391,-20352381,15557840,-31072141,-5019061,},
     {-6283632,-2259834,-4674247,-4598977,-4089240,
      12435688,-31278303,1060251,6256175,10480726,},
     {-13871026,2026300,-21928428,-2741605,-2406664,
      -8034988,7355518,15733500,-23379862,7489131,},},
    {{6883359,695140,23196907,9644202,-33430614,
      11354760,-20134606,6388313,-8263585,-8491918,},
     {-7716174,-13605463,-13646110,14757414,-19430591,
      -14967316,10359532,-11059670,-21935259,12082603,},
     {-11253345,-15943946,10046784,5414629,24840771,
      8086951,-6694742,9868723,15842692,-16224787,},},
    {{9639399,11810955,-24007778,-9320054,3912937,
      -9856959,996125,-8727907,-8919186,-14097242,},
     {7248867,14468564,25228636,-8795035,14346339,
      8224790,6388427,-7181107,6468218,-8720783,},
     {15513115,15439095,7342322,-10157390,18005294,
      -7265713,2186239,4884640,10826567,7135781,},},
    {{-14204238,5297536,-5862318,-6004934,28095835,
      4236101,-14203318,1958636,-16816875,3837147,},
     {-5511166,-13176782,-29588215,12339465,15325758,
      -15945770,-8813185,11075932,-19608050,-3776283,},
     {11728032,9603156,-4637821,-5304487,-7827751,
      2724948,31236191,-16760175,-7268616,14799772,},},
    {{-28842672,4840636,-12047946,-9101456,-1445464,
      381905,-30977094,-16523389,1290540,12798615,},
     {27246947,-10320914,14792098,-14518944,5302070,
      -8746152,-3403974,-4149637,-27061213,10749585,},
     {25572375,-6270368,-15353037,16037944,1146292,
      32198,23487090,9585613,24714571,-1418265,},},
    {{19844825,282124,-17583147,11004019,-32004269,
      -2716035,6105106,-1711007,-21010044,14338445,},
     {8027505,8191102,-18504907,-12335737,25173494,
      -5923905,15446145,7483684,-30440441,10009108,},
     {-14134701,-4174411,10246585,-14677495,33553567,
      -14012935,23366126,15080531,-7969992,7663473,},},
};

static const ge_precomp b_comb_high[8] = {
    {{33055887,-4431773,-521787,6654165,951411,
      -6266464,-5158124,6995613,-5397442,-6985227,},
     {4014062,6967095,-11977872,3960002,8001989,
      5130302,-2154812,-1899602,-31954493,-16173976,},
     {16271757,-9212948,23792794,731486,-25808309,
      -3546396,6964344,-4767590,10976593,10050757,},},
    {{2533007,-4288439,-24467768,-12387405,-13450051,
      14542280,12876301,13893535,15067764,8594792,},
     {20073501,-11623621,3165391,-13119866,13188608,
      -11540496,-10751437,-13482671,29588810,2197295,},
     {-1084082,11831693,6031797,14062724,14748428,
      -8159962,-20721760,11742548,31368706,13161200,},},
    {{2050412,-6457589,15321215,5273360,25484180,
      124590,-18187548,-7097255,-6691621,-14604792,},
     {9938196,2162889,-6158074,-1711248,4278932,
      -2598531,-22865792,-7168500,-24323168,11746309,},
     {-22691768,-14268164,5965485,9383325,20443693,
      5854192,28250679,-1381811,-10837134,13717818,},},
    {{-8495530,16382250,9548884,-4971523,-4491811,
      -3902147,6182256,-12832479,26628081,10395408,},
     {27329048,-15853735,7715764,8717446,-9215518,
      -14633480,28982250,-5668414,4227628,242148,},
     {-13279943,-7986904,-7100016,8764468,-27276630,
      3096719,29678419,-9141299,3906709,11265498,},},
    {{11918285,15686328,-17757323,-11217300,-27548967,
      4853165,-27168827,6807359,6871949,-1075745,},
     {-29002610,13984323,-27111812,-2713442,28107359,
      -13266203,6155126,15104658,3538727,-7513788,},
     {14103158,11233913,-33165269,9279850,31014152,
      4335090,-1827936,4590951,13960841,12787712,},},
    {{1469134,-16738009,33411928,13942824,8092558,
      -8778224,-11165065,1437842,22521552,-2792954,},
     {31352705,-4807352,-25327300,3962447,12541566,
      -9399651,-27425693,7964818,-23829869,5541287,},
     {-25732021,-6864887,23848984,3039395,-9147354,
      6022816,-27421653,10590137,25309915,-1584678,},},
    {{-22951376,5048948,31139401,-190316,-19542447,
      -626310,-17486305,-16511925,-18851313,-12985140,},
     {-9684890,14681754,30487568,7717771,-10829709,
      9630497,30290549,-10531496,-27798994,-13812825,},
     {5827835,16097107,-24501327,12094619,7413972,
      11447087,28057551,-1793987,-14056981,4359312,},},
    {{26323183,2342588,-21887793,-1623758,-6062284,
      2107090,-28724907,9036464,-19618351,-13055189,},
     {-29697200,14829398,-4596333,14220089,-30022969,
      2955645,12094100,-13693652,-5941445,7047569,},
     {-3201977,14413268,-12058324,-16417589,-9035655,
      -7224648,9258160,1399236,30397584,-5684634,},},
};

static void lookup_add(ge *p, ge_precomp *tmp_c, fe tmp_a, fe tmp_b,
                       const ge_precomp comb[8], const u8 scalar[32], int i)
{
    u8 teeth = (u8)((scalar_bit(scalar, i)          ) +
                    (scalar_bit(scalar, i + 32) << 1) +
                    (scalar_bit(scalar, i + 64) << 2) +
                    (scalar_bit(scalar, i + 96) << 3));
    u8 high  = teeth >> 3;
    u8 index = (teeth ^ (high - 1)) & 7;
    FOR (j, 0, 8) {
        i32 select = 1 & (((j ^ index) - 1) >> 8);
        fe_ccopy(tmp_c->Yp, comb[j].Yp, select);
        fe_ccopy(tmp_c->Ym, comb[j].Ym, select);
        fe_ccopy(tmp_c->T2, comb[j].T2, select);
    }
    fe_neg(tmp_a, tmp_c->T2);
    fe_cswap(tmp_c->T2, tmp_a    , high ^ 1);
    fe_cswap(tmp_c->Yp, tmp_c->Ym, high ^ 1);
    ge_madd(p, p, tmp_c, tmp_a, tmp_b);
}

// p = [scalar]B, where B is the base point
static void ge_scalarmult_base(ge *p, const u8 scalar[32])
{
    // twin 4-bits signed combs, from Mike Hamburg's
    // Fast and compact elliptic-curve cryptography (2012)
    // 1 / 2 modulo L
    static const u8 half_mod_L[32] = {
        247,233,122,46,141,49,9,44,107,206,123,81,239,124,111,10,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8, };
    // (2^256 - 1) / 2 modulo L
    static const u8 half_ones[32] = {
        142,74,204,70,186,24,118,107,184,231,190,57,250,173,119,99,
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,7, };

    // All bits set form: 1 means 1, 0 means -1
    u8 s_scalar[32];
    mul_add(s_scalar, scalar, half_mod_L, half_ones);

    // Double and add ladder
    fe tmp_a, tmp_b;  // temporaries for addition
    ge_precomp tmp_c; // temporary for comb lookup
    ge tmp_d;         // temporary for doubling
    fe_1(tmp_c.Yp);
    fe_1(tmp_c.Ym);
    fe_0(tmp_c.T2);

    // Save a double on the first iteration
    ge_zero(p);
    lookup_add(p, &tmp_c, tmp_a, tmp_b, b_comb_low , s_scalar, 31);
    lookup_add(p, &tmp_c, tmp_a, tmp_b, b_comb_high, s_scalar, 31+128);
    // Regular double & add for the rest
    for (int i = 30; i >= 0; i--) {
        ge_double(p, p, &tmp_d);
        lookup_add(p, &tmp_c, tmp_a, tmp_b, b_comb_low , s_scalar, i);
        lookup_add(p, &tmp_c, tmp_a, tmp_b, b_comb_high, s_scalar, i+128);
    }
    // Note: we could save one addition at the end if we assumed the
    // scalar fit in 252 bits.  Which it does in practice if it is
    // selected at random.  However, non-random, non-hashed scalars
    // *can* overflow 252 bits in practice.  Better account for that
    // than leaving that kind of subtle corner case.

    WIPE_BUFFER(tmp_a);  WIPE_CTX(&tmp_d);
    WIPE_BUFFER(tmp_b);  WIPE_CTX(&tmp_c);
    WIPE_BUFFER(s_scalar);
}

void crypto_sign_public_key_custom_hash(u8       public_key[32],
                                        const u8 secret_key[32],
                                        const crypto_sign_vtable *hash)
{
    u8 a[64];
    hash->hash(a, secret_key, 32);
    trim_scalar(a);
    ge A;
    ge_scalarmult_base(&A, a);
    ge_tobytes(public_key, &A);
    WIPE_BUFFER(a);
    WIPE_CTX(&A);
}

void crypto_sign_init_first_pass_custom_hash(crypto_sign_ctx_abstract *ctx,
                                             const u8 secret_key[32],
                                             const u8 public_key[32],
                                             const crypto_sign_vtable *hash)
{
    ctx->hash  = hash; // set vtable
    u8 *a      = ctx->buf;
    u8 *prefix = ctx->buf + 32;
    ctx->hash->hash(a, secret_key, 32);
    trim_scalar(a);

    if (public_key == 0) {
        crypto_sign_public_key_custom_hash(ctx->pk, secret_key, ctx->hash);
    } else {
        COPY(ctx->pk, public_key, 32);
    }

    // Deterministic part of EdDSA: Construct a nonce by hashing the message
    // instead of generating a random number.
    // An actual random number would work just fine, and would save us
    // the trouble of hashing the message twice.  If we did that
    // however, the user could fuck it up and reuse the nonce.
    ctx->hash->init  (ctx);
    ctx->hash->update(ctx, prefix , 32);
}

void crypto_sign_update(crypto_sign_ctx_abstract *ctx,
                        const u8 *msg, size_t msg_size)
{
    ctx->hash->update(ctx, msg, msg_size);
}

void crypto_sign_init_second_pass(crypto_sign_ctx_abstract *ctx)
{
    u8 *r        = ctx->buf + 32;
    u8 *half_sig = ctx->buf + 64;
    ctx->hash->final(ctx, r);
    reduce(r);

    // first half of the signature = "random" nonce times the base point
    ge R;
    ge_scalarmult_base(&R, r);
    ge_tobytes(half_sig, &R);
    WIPE_CTX(&R);

    // Hash R, the public key, and the message together.
    // It cannot be done in parallel with the first hash.
    ctx->hash->init  (ctx);
    ctx->hash->update(ctx, half_sig, 32);
    ctx->hash->update(ctx, ctx->pk , 32);
}

void crypto_sign_final(crypto_sign_ctx_abstract *ctx, u8 signature[64])
{
    u8 *a        = ctx->buf;
    u8 *r        = ctx->buf + 32;
    u8 *half_sig = ctx->buf + 64;
    u8  h_ram[64];
    ctx->hash->final(ctx, h_ram);
    reduce(h_ram);
    COPY(signature, half_sig, 32);
    mul_add(signature + 32, h_ram, a, r); // s = h_ram * a + r
    WIPE_BUFFER(h_ram);
    crypto_wipe(ctx, ctx->hash->ctx_size);
}

void crypto_check_init_custom_hash(crypto_check_ctx_abstract *ctx,
                                   const u8 signature[64],
                                   const u8 public_key[32],
                                   const crypto_sign_vtable *hash)
{
    ctx->hash = hash; // set vtable
    COPY(ctx->buf, signature , 64);
    COPY(ctx->pk , public_key, 32);
    ctx->hash->init  (ctx);
    ctx->hash->update(ctx, signature , 32);
    ctx->hash->update(ctx, public_key, 32);
}

void crypto_check_update(crypto_check_ctx_abstract *ctx,
                         const u8 *msg, size_t msg_size)
{
    ctx->hash->update(ctx, msg, msg_size);
}

int crypto_check_final(crypto_check_ctx_abstract *ctx)
{
    u8 h_ram[64];
    ctx->hash->final(ctx, h_ram);
    reduce(h_ram);
    u8 *R       = ctx->buf;      // R
    u8 *s       = ctx->buf + 32; // s
    u8 *R_check = ctx->pk;       // overwrite ctx->pk to save stack space
    if (ge_r_check(R_check, s, h_ram, ctx->pk)) {
        return -1;
    }
    return crypto_verify32(R, R_check); // R == R_check ? OK : fail
}

