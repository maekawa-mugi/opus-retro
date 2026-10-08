# EE MMI experimental optimization

This directory holds an opt-in, fixed-point-only R5900 Emotion Engine MMI
implementation of the CELT 16-bit inner product. The internal SILK
`silk_inner_prod_aligned()` path already calls `celt_inner_prod()` in
fixed-point builds, so it can also benefit.

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

Future optimizations: pitch xcorr, FIR and longer vector batches, only
after this first primitive passes EE hardware tests.
