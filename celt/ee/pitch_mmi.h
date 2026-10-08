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


/* CELT 4-lag correlation. Requires both 16-byte-aligned inputs and a
 * positive multiple-of-eight count. Its final second LQ reads y[i+8..i+15],
 * so the C caller must reserve eight or more samples as a scalar tail.
 * The real R5900 assembly is in xcorr_mmi.S.
 */
void opus_ee_xcorr_kernel_mmi_aligned8(const opus_val16 *x,
                                      const opus_val16 *y, int N,
                                      opus_val32 sum[4]);

static OPUS_INLINE void xcorr_kernel_ee_mmi(const opus_val16 *x,
                                           const opus_val16 *y,
                                           opus_val32 sum[4], int N)
{
   int i = 0;
#ifdef OPUS_CHECK_ASM
   opus_val32 expected[4] = {sum[0], sum[1], sum[2], sum[3]};
#endif
   /* EE LQ rounds its address DOWN to 16 bytes. Match the relative
    * alignment before peeling a scalar prefix; otherwise stay scalar.
    */
   if (N >= 16 && ((((unsigned long)x) ^ ((unsigned long)y)) & 15U) == 0)
   {
      while (i < N && (((unsigned long)(x+i)) & 15U) != 0)
      {
         opus_val16 v = x[i];
         sum[0] = MAC16_16(sum[0], v, y[i]);
         sum[1] = MAC16_16(sum[1], v, y[i+1]);
         sum[2] = MAC16_16(sum[2], v, y[i+2]);
         sum[3] = MAC16_16(sum[3], v, y[i+3]);
         ++i;
      }
      if (N-i >= 16)
      {
         /* The aligned MMI kernel loads 16 y samples per 8 x samples.
          * Leave >=8 scalar samples to avoid reading beyond y[N+3].
          */
         int count = (N-i-8) & ~7;
         opus_ee_xcorr_kernel_mmi_aligned8(x+i, y+i, count, sum);
         i += count;
      }
   }
   for (; i < N; ++i)
   {
      opus_val16 v = x[i];
      sum[0] = MAC16_16(sum[0], v, y[i]);
      sum[1] = MAC16_16(sum[1], v, y[i+1]);
      sum[2] = MAC16_16(sum[2], v, y[i+2]);
      sum[3] = MAC16_16(sum[3], v, y[i+3]);
   }
#ifdef OPUS_CHECK_ASM
   xcorr_kernel_c(x, y, expected, N);
   celt_assert(sum[0] == expected[0] && sum[1] == expected[1] &&
               sum[2] == expected[2] && sum[3] == expected[3]);
#endif
}

#define OVERRIDE_XCORR_KERNEL
#define xcorr_kernel(x, y, sum, len, arch) \
    ((void)(arch), xcorr_kernel_ee_mmi((x), (y), (sum), (len)))

#endif /* CELT_EE_PITCH_MMI_H */
