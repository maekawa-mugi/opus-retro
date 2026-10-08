/* EE R5900 fixed-point CELT inner product. No MMI instruction is emitted
 * unless OPUS_EE_MMI is explicitly enabled for a real EE toolchain.
 * Include from pitch.h AFTER celt_inner_prod_c() has been defined.
 */
#ifndef CELT_EE_PITCH_MMI_H
#define CELT_EE_PITCH_MMI_H

#if !defined(OPUS_EE_MMI) || !defined(FIXED_POINT)
#error "EE MMI pitch kernels require OPUS_EE_MMI and FIXED_POINT"
#endif

/* Number of elements must be positive and a multiple of eight; both input
 * addresses must be 16-byte aligned. Implemented in inner_prod_mmi.S.
 */
opus_val32 opus_ee_inner_prod_mmi_aligned8(const opus_val16 *x,
                                          const opus_val16 *y, int N);

static OPUS_INLINE opus_val32 celt_inner_prod_ee_mmi(const opus_val16 *x,
                                                     const opus_val16 *y,
                                                     int N)
{
   opus_val32 sum = 0;
   int i = 0;

   /* The EE LQ/SQ instructions round addresses DOWN to 16 bytes. They
    * must never be used with misaligned pointers. Both arrays must have
    * the same alignment so a scalar prefix can align them together.
    */
   if (N >= 16 &&
       ((((unsigned long)x) ^ ((unsigned long)y)) & 15U) == 0)
   {
      while (i < N && (((unsigned long)(x+i)) & 15U) != 0)
      {
         sum = MAC16_16(sum, x[i], y[i]);
         ++i;
      }
      if (N-i >= 16)
      {
         int blocks = (N-i) & ~7;
         opus_val32 vector_sum = opus_ee_inner_prod_mmi_aligned8(x+i, y+i,
                                                                 blocks);
         sum = ADD32_ovflw(sum, vector_sum);
         i += blocks;
      }
   }
   for (; i < N; ++i)
      sum = MAC16_16(sum, x[i], y[i]);

#ifdef OPUS_CHECK_ASM
   celt_assert(sum == celt_inner_prod_c(x, y, N));
#endif
   return sum;
}

#define OVERRIDE_CELT_INNER_PROD
#define celt_inner_prod(x, y, N, arch) \
    ((void)(arch), celt_inner_prod_ee_mmi((x), (y), (N)))

#endif /* CELT_EE_PITCH_MMI_H */
