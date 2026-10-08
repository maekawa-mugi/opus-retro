# EE MMI experimental optimization

This directory holds an opt-in, fixed-point-only R5900 Emotion Engine MMI
implementation of the CELT 16-bit inner product. The internal SILK
`silk_inner_prod_aligned()` path already calls `celt_inner_prod()` in
fixed-point builds, so it can also benefit. A second MMI kernel implements
SILK's exact 64-bit `silk_inner_prod16_c()` accumulation using `PMULTH`
and `PMFHL.UW`. Unlike the CELT kernel, it never drops carries between
32-bit pair products.

## Requirements and scope

- **CPU:** original PlayStation 2 Emotion Engine R5900 with MMI.
- **Toolchain:** PS2-capable assembler recognizing `lq`, `sq`, `phmadh`,
  `paddw`, and `pxor`. Generic MIPS32/MIPS64 toolchains are NOT suitable.
- **Configuration:** explicitly enable fixed-point and EE MMI.
- **Memory:** 16-bit signed PCM; 16-byte alignment is checked in C. A scalar
  prefix aligns pointers when both have the same offset modulo 16. Mismatched
  alignment and short vectors always use the original C arithmetic.
- **Numeric contract:** wrap to the low 32 bits of the signed 16x16 sum,
  matching the Opus fixed-point path. This is NOT a 64-bit Silk inner product.
  The separate SILK MMI kernel returns the *full* signed 64-bit sum.
- **ABI:** the handwritten assembly uses o32-style a0/a1/a2/v0 calling
  convention and caller-saved temporary registers. Test on the actual PS2
  toolchain, as it is not part of the generic upstream MIPS ISA.

### Build

Autotools (EE cross compiler and assembler):

```sh
./autogen.sh
CC=... ./configure --host=... --enable-fixed-point --enable-ee-mmi --enable-check-asm
make
```

CMake (supply an actual EE toolchain file):

```sh
cmake -S . -B build-ee -DCMAKE_TOOLCHAIN_FILE=ps2-toolchain.cmake \
  -DOPUS_FIXED_POINT=ON -DOPUS_EE_MMI=ON -DOPUS_CHECK_ASM=ON
cmake --build build-ee
```

Meson (using an EE cross file):

```sh
meson setup build-ee --cross-file ps2.ini -Dfixed-point=true -Dee-mmi=true
meson compile -C build-ee
```

All three features are **off by default**. A plain MIPS build continues
using the existing MIPS implementations. Never enable this for a non-EE CPU.

## Status and validation

The C wrapper and mathematical contract can be unit-tested on a host CPU
with a scalar stand-in for the assembly symbol. That checks vector-group
selection, aligned/misaligned fallbacks, extreme inputs and tails.

**The MMI instruction stream has not been assembled with a real R5900
toolchain or executed on PlayStation 2.** Treat it as an experimental
candidate until native tests confirm behavior and speed.

## Four-lag CELT correlation

`xcorr_mmi.S` evaluates four adjacent pitch-correlation lags in parallel.
Each iteration uses PHMADH/PADDW to accumulate four 32-bit partial sums per
lag, and MTSAH/QFSRV to shift the aligned input stream by 1, 2, and 3
halfwords. The final horizontal additions use modular 32-bit arithmetic.

`xcorr_kernel_ee_mmi()` is wired into CELT through `OVERRIDE_XCORR_KERNEL`.
It preserves pre-existing `sum[4]` values, processes a scalar prefix to
align both inputs together, and always leaves a scalar tail of at least
eight samples because the second aligned LQ reads eight extra y elements.
If x and y have different offsets modulo 16, the entire correlation uses
the reference C path. This is deliberately conservative: it never reads
before a buffer or beyond the documented y[N+3] window.

A host-only model test covers random/extreme inputs, 16-byte alignment
offsets, lengths, nonzero incoming sums, and bounds of the MMI loads.
Build and run with the standalone test in
`tests/test_ee_mmi_xcorr_host.c`. No physical R5900 MMI instructions
have been executed; native correctness and performance remain unverified.

Future optimizations: support mismatched stream alignment without unsafe
LQ access, reduce QFSRV scheduling overhead, and benchmark FIR/FFT kernels
on real EE hardware.
