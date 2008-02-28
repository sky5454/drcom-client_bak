/* let's try to implement the md5 algorithm without looking at the
   reference code... (so that this can become public domain) */
/* I only read http://en.wikipedia.org/wiki/MD5
   and of course the original rfc (but not the code) */
/* well, i actually read the code, but only to clear up ambiguous
   text in the rfc */
/* don't the comments look similar to those in the rfc? don't worry,
   i rewrote them all */
/* is it ok to use << in place of <<<? */
/* no, <<< is rol32 not << */
/* sh*t, the bug is in the driver not the function */
/* lets optimize */
/* i found another stupid bug (i < lrm - (n+1)) should be i < lrm */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "md5.h"

/* 0x12345678: 0x12 is the highest order byte */

#define rol32(r, s) \
( \
  ((r) << (s)) | \
  ((r & 0xffffffff) >> (32 - (s))) \
)

#define flip8(x) \
( \
  (((x) << 56) & 0xff00000000000000ull) | \
  (((x) << 40) & 0x00ff000000000000ull) | \
  (((x) << 24) & 0x0000ff0000000000ull) | \
  (((x) <<  8) & 0x000000ff00000000ull) | \
  (((x) >>  8) & 0x00000000ff000000ull) | \
  (((x) >> 24) & 0x0000000000ff0000ull) | \
  (((x) >> 40) & 0x000000000000ff00ull) | \
  (((x) >> 56) & 0x00000000000000ffull) \
)

#define flip4(x) \
( \
  (((x) << 24) & 0xff000000ull) | \
  (((x) <<  8) & 0x00ff0000ull) | \
  (((x) >>  8) & 0x0000ff00ull) | \
  (((x) >> 24) & 0x000000ffull) \
)

#define high8(x) \
(((x) >> 32) & 0xffffffff)

#define low8(x) \
((x) & 0xffffffff)

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define tolsb8(x) (x)
#define tomsb8(x) flip8(x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define tolsb8(x) flip8(x)
#define tomsb8(x) (x)
#else
#error No endianness???
#endif

/*
  assumptions:
  + message is a multiple of 8 bits
  + uint64_t is greater than or equal to unsigned long
  + n < 2^64
*/

/* precalculated: 64*8==512, 56*8==448 */

/* same prototype as openssl's MD5 */
unsigned char *MD5(const unsigned char *d, unsigned long n, unsigned char *md)
{
  uint8_t *rm = (uint8_t *) malloc(n + 64);
  uint64_t lrm, N, t64;
  uint32_t *M, *MD, T[65], X[16], i, j, A, B, C, D, AA, BB, CC, DD;

  /* Step 1. Append Padding Bits */

  t64 = n % 64;
  if (t64 == 56)
    lrm = n + 64;
  else if (t64 < 56)
    lrm = n + (56 - t64);
  else /* lrm > 56 */
    lrm = n - (t64 - 56) + 64;

  for (i = 0; i < n; ++i)
    rm[i] = d[i];
  rm[n] = 0x80; /* 10000000(2) */
  for (i = n + 1; i < lrm; ++i)
    rm[i] = 0;

  /* Step 2. Append Length */

  t64 = tolsb8((uint64_t) n);
  t64 <<= 3; /* *8 for the number of bits */
  *((u_int64_t *) (rm + lrm)) = t64;

  M = (uint32_t *) rm;
  N = (lrm + 8) / 4;

  /* Step 3. Initialize MD Buffer */

#if __BYTE_ORDER == __LITTLE_ENDIAN
  A = 0x67452301;
  B = 0xefcdab89;
  C = 0x98badcfe;
  D = 0x10325476;
#elif __BYTE_ORDER == __BIG_ENDIAN
  A = 0x01234567;
  B = 0x89abcdef;
  C = 0xfedcba98;
  D = 0x76543210;
#else
#error No endianness???
#endif

  /* Step 4. Process Message in 16-Word Blocks */

#define F(X,Y,Z) (((X) & (Y)) | (~(X) & (Z)))
#define G(X,Y,Z) (((X) & (Z)) | ((Y) & ~(Z)))
#define H(X,Y,Z) ((X) ^ (Y) ^ (Z))
#define I(X,Y,Z) ((Y) ^ ((X) | ~(Z)))

  for (i = 1; i <= 64; ++i)
    T[i] = (uint32_t) ((uint64_t) (0x100000000ull * fabs(sin(i))));

  /* Process each 16-word block */
  for (i = 0; i < N / 16; ++i)
  {
    /* Copy block i into X */
    for (j = 0; j < 16; ++j)
      X[j] = M[i * 16 + j];

    /* Save A as AA, B as BB, C as CC, and D as DD */
    AA = A;
    BB = B;
    CC = C;
    DD = D;

    /* Round 1. */

#define R1(a,b,c,d,k,s,i) (a += F(b,c,d) + X[k] + T[i], a = rol32(a,s) + b)

    R1(A,B,C,D, 0, 7, 1);
    R1(D,A,B,C, 1,12, 2);
    R1(C,D,A,B, 2,17, 3);
    R1(B,C,D,A, 3,22, 4);

    R1(A,B,C,D, 4, 7, 5);
    R1(D,A,B,C, 5,12, 6);
    R1(C,D,A,B, 6,17, 7);
    R1(B,C,D,A, 7,22, 8);

    R1(A,B,C,D, 8, 7, 9);
    R1(D,A,B,C, 9,12,10);
    R1(C,D,A,B,10,17,11);
    R1(B,C,D,A,11,22,12);

    R1(A,B,C,D,12, 7,13);
    R1(D,A,B,C,13,12,14);
    R1(C,D,A,B,14,17,15);
    R1(B,C,D,A,15,22,16);

    /* Round 2. */

#define R2(a,b,c,d,k,s,i) (a += G(b,c,d) + X[k] + T[i], a = rol32(a,s) + b)

    R2(A,B,C,D, 1, 5,17);
    R2(D,A,B,C, 6, 9,18);
    R2(C,D,A,B,11,14,19);
    R2(B,C,D,A, 0,20,20);

    R2(A,B,C,D, 5, 5,21);
    R2(D,A,B,C,10, 9,22);
    R2(C,D,A,B,15,14,23);
    R2(B,C,D,A, 4,20,24);

    R2(A,B,C,D, 9, 5,25);
    R2(D,A,B,C,14, 9,26);
    R2(C,D,A,B, 3,14,27);
    R2(B,C,D,A, 8,20,28);

    R2(A,B,C,D,13, 5,29);
    R2(D,A,B,C, 2, 9,30);
    R2(C,D,A,B, 7,14,31);
    R2(B,C,D,A,12,20,32);

    /* Round 3. */

#define R3(a,b,c,d,k,s,i) (a += H(b,c,d) + X[k] + T[i], a = rol32(a,s) + b)

    R3(A,B,C,D, 5, 4,33);
    R3(D,A,B,C, 8,11,34);
    R3(C,D,A,B,11,16,35);
    R3(B,C,D,A,14,23,36);

    R3(A,B,C,D, 1, 4,37);
    R3(D,A,B,C, 4,11,38);
    R3(C,D,A,B, 7,16,39);
    R3(B,C,D,A,10,23,40);

    R3(A,B,C,D,13, 4,41);
    R3(D,A,B,C, 0,11,42);
    R3(C,D,A,B, 3,16,43);
    R3(B,C,D,A, 6,23,44);

    R3(A,B,C,D, 9, 4,45);
    R3(D,A,B,C,12,11,46);
    R3(C,D,A,B,15,16,47);
    R3(B,C,D,A, 2,23,48);

    /* Round 4. */

#define R4(a,b,c,d,k,s,i) (a += I(b,c,d) + X[k] + T[i], a = rol32(a,s) + b)

    R4(A,B,C,D, 0, 6,49);
    R4(D,A,B,C, 7,10,50);
    R4(C,D,A,B,14,15,51);
    R4(B,C,D,A, 5,21,52);

    R4(A,B,C,D,12, 6,53);
    R4(D,A,B,C, 3,10,54);
    R4(C,D,A,B,10,15,55);
    R4(B,C,D,A, 1,21,56);

    R4(A,B,C,D, 8, 6,57);
    R4(D,A,B,C,15,10,58);
    R4(C,D,A,B, 6,15,59);
    R4(B,C,D,A,13,21,60);

    R4(A,B,C,D, 4, 6,61);
    R4(D,A,B,C,11,10,62);
    R4(C,D,A,B, 2,15,63);
    R4(B,C,D,A, 9,21,64);

    A = A + AA;
    B = B + BB;
    C = C + CC;
    D = D + DD;
  }

  MD = (uint32_t *) md;
  MD[0] = A;
  MD[1] = B;
  MD[2] = C;
  MD[3] = D;

  free(rm);

  return md;
}

