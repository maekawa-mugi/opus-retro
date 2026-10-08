/*
 * Portable model of the EE MMI 64-bit SILK inner product wrapper.
 *
 * This executes the real wrapper but models PMULTH/PMFHL.UW using C.
 * The original EE instruction stream is NOT run by this test.
 *
 * cc -O2 -std=c99 -Wall -Wextra -Werror -DOPUS_CHECK_ASM=1 \
 *   tests/test_ee_mmi_inner_prod64_host.c -o test-ee64 && ./test-ee64
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef int16_t opus_int16;
typedef int64_t opus_int64;
#define OPUS_INLINE inline
#define FIXED_POINT 1
#define OPUS_EE_MMI 1
#define celt_assert(x) do { if (!(x)) abort(); } while (0)

#include "../silk/ee/vector_ops_mmi.h"

static int mmicalls;
void opus_ee_inner_prod64_mmi_aligned8(const opus_int16 *x,
                                      const opus_int16 *y, int n,
                                      opus_int64 *out)
{
   int i;
   int64_t result = 0;
   if (!n || (n&7) ||
       ((((unsigned long)x) | ((unsigned long)y)) & 15U))
      abort();
   ++mmicalls;
   /* PMULTH returns even products in rd and PMFHL.UW the odd products. */
   for (i=0; i<n; i+=8)
   {
      int32_t even[4], odd[4];
      int j;
      for (j=0; j<4; ++j)
      {
         even[j] = (int32_t)x[i+j*2] * y[i+j*2];
         odd[j]  = (int32_t)x[i+j*2+1] * y[i+j*2+1];
      }
      for (j=0; j<4; ++j) result += (int64_t)even[j] + odd[j];
   }
   *out = result;
}

static uint32_t state = UINT32_C(0x87654321);
static uint32_t random32(void)
{
   state = state * UINT32_C(1664525) + UINT32_C(1013904223);
   return state;
}
static opus_int16 a[512] __attribute__((aligned(16)));
static opus_int16 b[512] __attribute__((aligned(16)));
int main(void)
{
   int iter, xoff, yoff, n;
   unsigned tested=0, usedmmi=0, usedscalar=0;
   for (iter=0; iter<10; iter++)
   {
      int k;
      for (k=0; k<512; k++)
      {
         a[k] = (opus_int16)(random32() >> 16);
         b[k] = (opus_int16)(random32() >> 16);
      }
      if (iter==0)
      {
         for (k=0; k<512; k++)
         {
            a[k] = (opus_int16)(k & 1 ? INT16_MIN : INT16_MAX);
            b[k] = (opus_int16)(k & 2 ? INT16_MIN : INT16_MAX);
         }
      }
      for (xoff=0; xoff<8; xoff++)
      for (yoff=0; yoff<8; yoff++)
      for (n=0; n<249; n++)
      {
         int i, before = mmicalls;
         int64_t expected = 0;
         opus_int64 actual;
         for (i=0; i<n; i++)
            expected += (int64_t)a[xoff+i] * b[yoff+i];
         actual = silk_inner_prod16_ee_mmi(a+xoff, b+yoff, n);
         if (actual != expected)
         {
            fprintf(stderr, "FAIL: n=%d offs=%d/%d iter=%d got=%lld expected=%lld\n",
                    n, xoff, yoff, iter, (long long)actual, (long long)expected);
            return 1;
         }
         ++tested;
         if (mmicalls != before) ++usedmmi;
         else ++usedscalar;
      }
   }
   printf("PASS: %u 64-bit vectors; %u MMI-model; %u C fallback\n",
          tested, usedmmi, usedscalar);
   return usedmmi==0 || usedscalar==0;
}
