/*
 * R5900 MMI inner product for SILK's 64-bit accumulation.
 * Used only in a FIXED_POINT + OPUS_EE_MMI build.
 * This header requires silk typedefs and arithmetic helpers from SILK.
 */
#ifndef SILK_EE_VECTOR_OPS_MMI_H
#define SILK_EE_VECTOR_OPS_MMI_H

#if !defined(OPUS_EE_MMI) || !defined(FIXED_POINT)
#error "SILK EE MMI optimization requires fixed-point EE build"
#endif

void opus_ee_inner_prod64_mmi_aligned8(const opus_int16 *x,
                                      const opus_int16 *y,
                                      int N, opus_int64 *out);

static OPUS_INLINE opus_int64 silk_inner_prod16_ee_mmi(const opus_int16 *x,
                                                       const opus_int16 *y,
                                                       int N)
{
   opus_int64 sum = 0;
   int i = 0;
   if (N >= 16 &&
       ((((unsigned long)x) ^ ((unsigned long)y)) & 15U) == 0)
   {
      while (i < N && (((unsigned long)(x+i)) & 15U) != 0)
      {
         sum += (opus_int64)x[i] * y[i];
         ++i;
      }
      if (N-i >= 16)
      {
         opus_int64 vector_sum;
         int blocks = (N-i) & ~7;
         opus_ee_inner_prod64_mmi_aligned8(x+i, y+i, blocks, &vector_sum);
         sum += vector_sum;
         i += blocks;
      }
   }
   for (; i < N; ++i)
      sum += (opus_int64)x[i] * y[i];

#ifdef OPUS_CHECK_ASM
   {
      opus_int64 reference = 0;
      int j;
      for (j=0; j<N; ++j)
         reference += (opus_int64)x[j] * y[j];
      celt_assert(sum == reference);
   }
#endif
   return sum;
}
#endif /* SILK_EE_VECTOR_OPS_MMI_H */
