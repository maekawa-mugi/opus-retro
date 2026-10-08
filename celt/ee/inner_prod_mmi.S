/*
 * Emotion Engine (R5900) MMI fixed-point inner product.
 *
 * Inputs (o32): a0 = x, a1 = y, a2 = number of int16 elements.
 * Preconditions: x and y are 16-byte aligned, count is positive and
 * divisible by eight. The C wrapper enforces these requirements.
 *
 * Computes the low 32 bits of sum(x[i] * y[i]), exactly like the
 * fixed-point CELT inner product. phmadh overwrites HI/LO, which are
 * caller-clobbered under the EE ABI. All other registers used are
 * caller-saved.
 */
    .text
    .align  4
    .set    noreorder
    .globl  opus_ee_inner_prod_mmi_aligned8
    .ent    opus_ee_inner_prod_mmi_aligned8
opus_ee_inner_prod_mmi_aligned8:
    /* Reserve local scratch and align it independently of the caller ABI. */
    addiu   $sp, $sp, -48
    addiu   $t6, $sp, 15
    addiu   $t7, $zero, -16
    and     $t6, $t6, $t7

    pxor    $t3, $t3, $t3
    srl     $t5, $a2, 3

1:
    lq      $t0, 0($a0)
    lq      $t1, 0($a1)
    phmadh  $t2, $t0, $t1
    /* PHMADH produces four adjacent-pair sums in 32-bit lanes. */
    paddw   $t3, $t3, $t2

    addiu   $a0, $a0, 16
    addiu   $a1, $a1, 16
    addiu   $t5, $t5, -1
    bne     $t5, $zero, 1b
    nop

    sq      $t3, 0($t6)
    lw      $t0, 0($t6)
    lw      $t1, 4($t6)
    lw      $t2, 8($t6)
    lw      $t3, 12($t6)
    addu    $v0, $t0, $t1
    addu    $v0, $v0, $t2
    addu    $v0, $v0, $t3

    addiu   $sp, $sp, 48
    jr      $ra
    nop
    .end    opus_ee_inner_prod_mmi_aligned8
