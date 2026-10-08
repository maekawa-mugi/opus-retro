/*
 * Portable model test for the EE MMI CELT wrapper. This deliberately does
 * NOT execute EE instructions. It exercises the actual pitch_mmi.h dispatch,
 * scalar prefix/tail handling and the 32-bit modular arithmetic contract.
 *
 * cc -O2 -std=c99 -Wall -Wextra -Werror tests/test_ee_mmi_inner_prod_host.c -o test-ee
 * ./test-ee
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int16_t opus_val16;
typedef int32_t opus_val32;
#define OPUS_INLINE inline
#define FIXED_POINT 1
#define OPUS_EE_MMI 1
#define ADD32_ovflw(a,b) ((opus_val32)((uint32_t)(a)+(uint32_t)(b)))
#define MAC16_16(c,a,b) ADD32_ovflw((c), (opus_val32)((int32_t)(a)*(int32_t)(b)))
#define celt_assert(x) do { if (!(x)) abort(); } while (0)

/* This local reference is bit-precise even when the signed sum overflows. */
static opus_val32 celt_inner_prod_c(const opus_val16 *x,
                                    const opus_val16 *y, int n)
{
   uint32_t sum = 0;
   int i;
   for (i=0; i<n; i++)
      sum += (uint32_t)((int32_t)x[i] * (int32_t)y[i]);
   return (opus_val32)sum;
}
#include "../celt/ee/pitch_mmi.h"

static int vector_calls;
opus_val32 opus_ee_inner_prod_mmi_aligned8(const opus_val16 *x,
                                          const opus_val16 *y, int n)
{
   uint32_t lane[4] = {0, 0, 0, 0};
   int i;
   if (!n || (n & 7) ||
       ((((unsigned long)x) | ((unsigned long)y)) & 15U))
      abort();
   ++vector_calls;
   for (i=0; i<n; i+=8)
   {
      int j;
      for (j=0; j<4; j++)
      {
         lane[j] += (uint32_t)((int32_t)x[i+j*2]*y[i+j*2]);
         lane[j] += (uint32_t)((int32_t)x[i+j*2+1]*y[i+j*2+1]);
      }
   }
   return (opus_val32)(lane[0]+lane[1]+lane[2]+lane[3]);
}
static uint32_t rng_state = UINT32_C(0x712fe4c3);
static uint32_t next_random(void)
{
   rng_state = rng_state * UINT32_C(1664525) + UINT32_C(1013904223);
   return rng_state;
}
static opus_val16 x[512] __attribute__((aligned(16)));
static opus_val16 y[512] __attribute__((aligned(16)));
int main(void)
{
   int length, xa, ya, rep;
   unsigned tests = 0, vectorized = 0, fallback = 0;
   for (rep=0; rep<10; rep++)
   {
      int k;
      for (k=0; k<512; k++)
      {
         x[k] = (opus_val16)(next_random() >> 16);
         y[k] = (opus_val16)(next_random() >> 16);
      }
      if (rep==0)
      {
         for (k=0; k<512; k++)
         {
            x[k] = (opus_val16)(k & 1 ? INT16_MIN : INT16_MAX);
            y[k] = (opus_val16)(k & 2 ? INT16_MAX : INT16_MIN);
         }
      }
      for (xa=0; xa<8; xa++)
      for (ya=0; ya<8; ya++)
      for (length=0; length<249; length++)
      {
         int before = vector_calls;
         opus_val32 actual = celt_inner_prod(x+xa, y+ya, length, 0);
         opus_val32 expected = celt_inner_prod_c(x+xa, y+ya, length);
         if (actual != expected)
         {
            fprintf(stderr, "FAIL n=%d xa=%d ya=%d rep=%d: %d != %d\n",
                    length, xa, ya, rep, actual, expected);
            return 1;
         }
         ++tests;
         if (vector_calls != before) ++vectorized;
         else ++fallback;
      }
   }
   printf("PASS: %u vectors; %u used MMI model; %u scalar fallback\n",
          tests, vectorized, fallback);
   if (vectorized==0 || fallback==0) return 1;
   return 0;
}
