/*
 * Emotion Engine R5900 MMI: four CELT signed 16-bit correlations in parallel.
 *
 * void opus_ee_xcorr_kernel_mmi_aligned8(const int16_t *x,
 *                                       const int16_t *y,
 *                                       int n, int32_t sum[4]);
 *
 * Required by the C wrapper:
 *  - x and y are both 16-byte aligned.
 *  - n is a positive multiple of 8.
 *  - x has n samples; y has at least n+8 samples available (final LQ
 *    loads y[i+8..i+15], even though max lag needs only y[i+10]).
 *  - sum holds four pre-existing 32-bit modular accumulators.
 *
 * PHMADH produces four 32-bit adjacent-pair sums. PADDW accumulates
 * those independently. QFSRV / MTSAH generates shifted y windows.
 * Uses caller-saved GPRs and HI/LO, modifies the EE SA register.
 *
 * R5900 assembler and native hardware validation are still required.
 */
    .text
    .align  4
    .set    noreorder
    .globl  opus_ee_xcorr_kernel_mmi_aligned8
    .ent    opus_ee_xcorr_kernel_mmi_aligned8
opus_ee_xcorr_kernel_mmi_aligned8:
    addiu   $sp, $sp, -80
    addiu   $t9, $sp, 15
    addiu   $v1, $zero, -16
    and     $t9, $t9, $v1

    pxor    $t3, $t3, $t3
    pxor    $t4, $t4, $t4
    pxor    $t5, $t5, $t5
    pxor    $t6, $t6, $t6
1:
    mtsah   $zero, 1
    lq      $t0, 0($a0)
    lq      $t1, 0($a1)
    lq      $t2, 16($a1)
    phmadh  $t7, $t0, $t1
    paddw   $t3, $t3, $t7

    qfsrv   $t8, $t2, $t1
    phmadh  $t7, $t0, $t8
    paddw   $t4, $t4, $t7
    addiu   $a0, $a0, 16
    addiu   $a1, $a1, 16

    mtsah   $zero, 2
    qfsrv   $t8, $t2, $t1
    phmadh  $t7, $t0, $t8
    paddw   $t5, $t5, $t7
    addiu   $a2, $a2, -8

    mtsah   $zero, 3
    qfsrv   $t8, $t2, $t1
    phmadh  $t7, $t0, $t8
    paddw   $t6, $t6, $t7
    bne     $a2, $zero, 1b
    nop

    /* 64-byte scratch for the four aligned vector sums. */
    sq      $t3, 0($t9)
    sq      $t4, 16($t9)
    sq      $t5, 32($t9)
    sq      $t6, 48($t9)

    lw      $t0, 0($t9)
    lw      $t1, 4($t9)
    lw      $t2, 8($t9)
    lw      $t3, 12($t9)
    addu    $t0, $t0, $t1
    addu    $t2, $t2, $t3
    addu    $t0, $t0, $t2
    lw      $t1, 0($a3)
    addu    $t0, $t0, $t1
    sw      $t0, 0($a3)

    lw      $t0, 16($t9)
    lw      $t1, 20($t9)
    lw      $t2, 24($t9)
    lw      $t3, 28($t9)
    addu    $t0, $t0, $t1
    addu    $t2, $t2, $t3
    addu    $t0, $t0, $t2
    lw      $t1, 4($a3)
    addu    $t0, $t0, $t1
    sw      $t0, 4($a3)

    lw      $t0, 32($t9)
    lw      $t1, 36($t9)
    lw      $t2, 40($t9)
    lw      $t3, 44($t9)
    addu    $t0, $t0, $t1
    addu    $t2, $t2, $t3
    addu    $t0, $t0, $t2
    lw      $t1, 8($a3)
    addu    $t0, $t0, $t1
    sw      $t0, 8($a3)

    lw      $t0, 48($t9)
    lw      $t1, 52($t9)
    lw      $t2, 56($t9)
    lw      $t3, 60($t9)
    addu    $t0, $t0, $t1
    addu    $t2, $t2, $t3
    addu    $t0, $t0, $t2
    lw      $t1, 12($a3)
    addu    $t0, $t0, $t1
    sw      $t0, 12($a3)

    addiu   $sp, $sp, 80
    jr      $ra
    nop
    .end    opus_ee_xcorr_kernel_mmi_aligned8
