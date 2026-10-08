/*
 * Host-only dispatch/reference test for R5900 MMI four-lag correlation.
 * The MMI microkernel is modeled in C, NOT executed by the host CPU.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int16_t opus_val16;
typedef int32_t opus_val32;
#define OPUS_INLINE inline
#define FIXED_POINT 1
#define OPUS_EE_MMI 1
#define ADD32_ovflw(a,b) ((opus_val32)((uint32_t)(a)+(uint32_t)(b)))
#define MAC16_16(c,a,b) ADD32_ovflw((c), (opus_val32)((int32_t)(a)*(int32_t)(b)))
#define celt_assert(c) do { if (!(c)) abort(); } while (0)

static void xcorr_kernel_c(const opus_val16 *x, const opus_val16 *y,
                           opus_val32 sum[4], int len)
{
   int i, j;
   for (i=0; i<len; i++)
      for (j=0; j<4; j++)
         sum[j] = MAC16_16(sum[j], x[i], y[i+j]);
}

static opus_val32 celt_inner_prod_c(const opus_val16 *x,
                                    const opus_val16 *y, int n)
{
   opus_val32 sum=0;
   int i;
   for(i=0;i<n;i++) sum=MAC16_16(sum,x[i],y[i]);
   return sum;
}
#include "../celt/ee/pitch_mmi.h"

static int mmicalls;
static size_t model_x_bytes, model_y_bytes;
static const opus_val16 *start_x, *start_y;
void opus_ee_xcorr_kernel_mmi_aligned8(const opus_val16 *x,
                                      const opus_val16 *y, int count,
                                      opus_val32 sum[4])
{
   uint32_t accum[4][4] = {{0}};
   int i,lag,lane;
   if (count < 8 || count % 8 ||
       (((uintptr_t)x | (uintptr_t)y) & 15)) abort();
   if ((const unsigned char*)(x+count) >
       (const unsigned char*)start_x + model_x_bytes) abort();
   /* Model the WHOLE second 128-bit LQ, including its lookahead. */
   if ((const unsigned char*)(y+count+8) >
       (const unsigned char*)start_y + model_y_bytes) abort();
   mmicalls++;
   for (i=0; i<count; i+=8)
      for (lag=0; lag<4; lag++)
         for (lane=0; lane<4; lane++)
         {
            int k = i + lane*2;
            uint32_t pair = (uint32_t)((int32_t)x[k]*y[k+lag]);
            pair += (uint32_t)((int32_t)x[k+1]*y[k+lag+1]);
            accum[lag][lane] += pair;
         }
   for (lag=0; lag<4; lag++)
   {
      uint32_t total = (uint32_t)sum[lag];
      for (lane=0; lane<4; lane++) total += accum[lag][lane];
      sum[lag] = (opus_val32)total;
   }
}

static uint32_t state = UINT32_C(0x9e3779b9);
static uint32_t rand32(void)
{
   state ^= state << 13; state ^= state >> 17; state ^= state << 5;
   return state;
}
static opus_val16 xdata[560] __attribute__((aligned(16)));
static opus_val16 ydata[560] __attribute__((aligned(16)));
int main(void)
{
   unsigned tests=0, vec=0, fallback=0;
   int rep, i, xo, yo, n;
   for(rep=0;rep<8;rep++)
   {
      for(i=0;i<560;i++)
      {
         xdata[i] = (opus_val16)rand32();
         ydata[i] = (opus_val16)rand32();
         if(rep==0)
         {
            xdata[i] = i&1 ? INT16_MIN : INT16_MAX;
            ydata[i] = i&2 ? INT16_MAX : INT16_MIN;
         }
      }
      for(xo=0;xo<8;xo++)
      for(yo=0;yo<8;yo++)
      for(n=3;n<=270;n++)
      {
         opus_val32 actual[4], reference[4];
         int before=mmicalls;
         start_x=xdata+xo; start_y=ydata+yo;
         model_x_bytes=(size_t)n*2;
         model_y_bytes=(size_t)(n+3)*2;
         for(i=0;i<4;i++) actual[i]=reference[i]=(opus_val32)rand32();
         xcorr_kernel(start_x,start_y,actual,n,0);
         xcorr_kernel_c(start_x,start_y,reference,n);
         if(memcmp(actual,reference,sizeof(actual)))
         {
            fprintf(stderr,"FAIL rep=%d n=%d off=%d/%d\n",rep,n,xo,yo);
            return 1;
         }
         tests++;
         if(mmicalls!=before)vec++;
         else fallback++;
      }
   }
   printf("PASS %u xcorr tests; modeled MMI %u; fallback %u\n",
          tests,vec,fallback);
   return vec==0 || fallback==0;
}
