/*
 * EE R5900 MMI exact 64-bit signed inner product of int16 vectors.
 *
 * o32 arguments: a0=x, a1=y, a2=count, a3=pointer to int64 output.
 * Preconditions: x/y aligned 16 bytes, count positive multiple of 8,
 * output properly aligned for a 64-bit store. The SILK wrapper ensures
 * these. Uses only caller-saved registers and clobbers HI/LO.
 *
 * PMULTH exposes even-numbered signed 32-bit products in rd; PMFHL.UW
 * exposes the remaining odd products. Accumulation uses signed LW plus
 * 64-bit DADDU, so no pairwise 32-bit overflow can change the answer.
 */
    .text
    .align  4
    .set    noreorder
    .globl  opus_ee_inner_prod64_mmi_aligned8
    .ent    opus_ee_inner_prod64_mmi_aligned8
opus_ee_inner_prod64_mmi_aligned8:
    /* Two 16-byte scratch vectors; align without assuming stack ABI. */
    addiu   $sp, $sp, -64
    addiu   $t6, $sp, 15
    addiu   $t7, $zero, -16
    and     $t6, $t6, $t7

    daddu   $v0, $zero, $zero
    srl     $t5, $a2, 3
1:
    lq      $t0, 0($a0)
    lq      $t1, 0($a1)
    pmulth  $t2, $t0, $t1
    pmfhl.uw $t3
    sq      $t2, 0($t6)
    sq      $t3, 16($t6)

    lw      $t0, 0($t6)
    lw      $t1, 4($t6)
    lw      $t2, 8($t6)
    lw      $t3, 12($t6)
    daddu   $v0, $v0, $t0
    daddu   $v0, $v0, $t1
    daddu   $v0, $v0, $t2
    daddu   $v0, $v0, $t3
    lw      $t0, 16($t6)
    lw      $t1, 20($t6)
    lw      $t2, 24($t6)
    lw      $t3, 28($t6)
    daddu   $v0, $v0, $t0
    daddu   $v0, $v0, $t1
    daddu   $v0, $v0, $t2
    daddu   $v0, $v0, $t3

    addiu   $a0, $a0, 16
    addiu   $a1, $a1, 16
    addiu   $t5, $t5, -1
    bne     $t5, $zero, 1b
    nop

    sd      $v0, 0($a3)
    addiu   $sp, $sp, 64
    jr      $ra
    nop
    .end    opus_ee_inner_prod64_mmi_aligned8
