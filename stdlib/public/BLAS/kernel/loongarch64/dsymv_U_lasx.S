/*******************************************************************************
Copyright (c) 2024, The OpenBLAS Project
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
3. Neither the name of the OpenBLAS project nor the names of
its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE OPENBLAS PROJECT OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#define ASSEMBLER

#include "common.h"
#include "loongarch64_asm.S"

/* Param */
#define M      $r4
#define N      $r5
#define A      $r6
#define LDA    $r7
#define X      $r8
#define INCX   $r9
#define Y      $r10
#define INCY   $r11
#define BUFFER $r16
#define ALPHA  $f0

#define JY     $r18
#define JX     $r31
#define T0     $r19
#define T1     $r20
#define M1     $r12
#define AO4    $r13
#define I      $r14
#define J      $r15
#define AO1    $r23
#define AO2    $r24
#define IX     $r25
#define IY     $r26
#define II     $r27
#define T2     $r28
#define T3     $r29
#define T4     $r30
#define T5     $r17
#define T6     $r16

/* LSX vectors */
#define U0     $xr31
#define U1     $xr1
#define U2     $xr2
#define U3     $xr3
#define U4     $xr4
#define U5     $xr5
#define U6     $xr6
#define U7     $xr7
#define U8     $xr8
#define U9     $xr9
#define U10    $xr10
#define U11    $xr11
#define U12    $xr12
#define U13    $xr13
#define U14    $xr14
#define U15    $xr15
#define U16    $xr16
#define VALPHA $xr17

#define a2     $f2
#define a3     $f3
#define a4     $f4
#define a5     $f5
#define a6     $f6
#define a7     $f7
#define a8     $f8
#define a9     $f9

.macro LOAD_Y_8
    beqz T5, .L01_Y_0
    fldx.d      $f4,   Y,     IY
    add.d       T2,    IY,    INCY
    fldx.d      $f5,   Y,     T2
    add.d       T2,    T2,    INCY
    fldx.d      $f6,   Y,     T2
    add.d       T2,    T2,    INCY
    fldx.d      $f7,   Y,     T2

    add.d       T2,    T2,    INCY
    fldx.d      $f8,   Y,     T2
    add.d       T2,    T2,    INCY
    fldx.d      $f9,   Y,     T2
    add.d       T2,    T2,    INCY
    fldx.d      $f10,  Y,     T2
    add.d       T2,    T2,    INCY
    fldx.d      $f11,  Y,     T2

    vextrins.d  $vr4,    $vr5,    0x10
    vextrins.d  $vr6,    $vr7,    0x10
    xvpermi.q   U4,      U6,      0x02

    vextrins.d  $vr8,    $vr9,    0x10
    vextrins.d  $vr10,   $vr11,   0x10
    xvpermi.q   U8,      U10,     0x02
    b .L01_Y_1
.L01_Y_0:
    xvldx U4, Y,  IY
    alsl.d T4, INCY, IY, 2
    xvldx U8, Y,  T4
.L01_Y_1:
.endm

.macro STORE_Y_8
    beqz T5, .L01_Y_2
    xvpermi.d   U6,      U4,      0xee
    vextrins.d  $vr5,    $vr4,    0x01
    vextrins.d  $vr7,    $vr6,    0x01

    xvpermi.d   U10,     U8,      0xee
    vextrins.d  $vr9,    $vr8,    0x01
    vextrins.d  $vr11,   $vr10,   0x01

    fstx.d      $f4,   Y,     IY
    add.d       T2,    IY,    INCY
    fstx.d      $f5,   Y,     T2
    add.d       T2,    T2,    INCY
    fstx.d      $f6,   Y,     T2
    add.d       T2,    T2,    INCY
    fstx.d      $f7,   Y,     T2

    add.d       T2,    T2,    INCY
    fstx.d      $f8,   Y,     T2
    add.d       T2,    T2,    INCY
    fstx.d      $f9,   Y,     T2
    add.d       T2,    T2,    INCY
    fstx.d      $f10,  Y,     T2
    add.d       T2,    T2,    INCY
    fstx.d      $f11,  Y,     T2
    b .L01_Y_3
.L01_Y_2:
    xvstx  U4, Y, IY
    xvstx  U8, Y, T4
.L01_Y_3:
.endm

.macro LOAD_X_8
    beqz T6, .L01_X_0
    fldx.d      $f4,   X,     IX
    add.d       T2,    IX,    INCX
    fldx.d      $f5,   X,     T2
    add.d       T2,    T2,    INCX
    fldx.d      $f6,   X,     T2
    add.d       T2,    T2,    INCX
    fldx.d      $f7,   X,     T2

    add.d       T2,    T2,    INCX
    fldx.d      $f8,   X,     T2
    add.d       T2,    T2,    INCX
    fldx.d      $f9,   X,     T2
    add.d       T2,    T2,    INCX
    fldx.d      $f10,  X,     T2
    add.d       T2,    T2,    INCX
    fldx.d      $f11,  X,     T2

    vextrins.d  $vr4,    $vr5,    0x10
    vextrins.d  $vr6,    $vr7,    0x10
    xvpermi.q   U4,      U6,      0x02

    vextrins.d  $vr8,    $vr9,    0x10
    vextrins.d  $vr10,   $vr11,   0x10
    xvpermi.q   U8,      U10,     0x02
    b .L01_X_1
.L01_X_0:
    xvldx  U4, X,  IX
    alsl.d T2, INCX, IX, 2
    xvldx  U8, X,  T2
.L01_X_1:
.endm

    PROLOGUE

    addi.d  $sp, $sp, -88

    SDARG  $r23,  $sp,    0
    SDARG  $r24,  $sp,    8
    SDARG  $r25,  $sp,    16
    SDARG  $r26,  $sp,    32
    SDARG  $r27,  $sp,    40
    SDARG  $r28,  $sp,    48
    SDARG  $r29,  $sp,    56
    SDARG  $r30,  $sp,    64
    SDARG  $r31,  $sp,    72
    ST     ALPHA, $sp,    80

    xvldrepl.d  VALPHA, $sp, 80

    addi.d     T5,      INCY, -1
    addi.d     T6,      INCX, -1
    slli.d     LDA,     LDA,  BASE_SHIFT
    slli.d     INCX,    INCX, BASE_SHIFT
    slli.d     INCY,    INCY, BASE_SHIFT

    bge    $r0,    M,    .L999
    bge    $r0,    N,    .L999

    sub.d  M1,     M,    N

    mul.d   JY,     M1,    INCY
    mul.d   JX,     M1,    INCX

    move   J,      M1
    move   AO1,    A

    beq        J,      M,    .L999

.L01:
    xvxor.v     U2,    U2,  U2
    fldx.d      $f6,   X,   JX
    fmul.d      $f3,   ALPHA, $f6  //temp1
    xvreplve0.d U3,   U3

    move      IY,     $r0
    move      IX,     $r0
    move      II,     $r0
    move      I,      $r0

    srai.d    T0,     J,   3
    beq       I,      T0,    .L03

    mul.d       T1,     J,    LDA
    add.d       T1,     T1,   II

.L02:  /* /8 */
    xvldx       U1,    AO1,   T1
    addi.d      T2,     T1,   32
    xvldx       U14,   AO1,   T2

    LOAD_Y_8

    xvfmadd.d    U4,    U3,    U1,    U4
    xvfmadd.d    U8,    U3,    U14,   U8

    STORE_Y_8

    alsl.d IY, INCY, IY, 3

    LOAD_X_8

    xvfmadd.d    U2,    U1,    U4,    U2
    xvfmadd.d    U2,    U14,   U8,    U2

    alsl.d IX, INCX, IX, 3

    addi.d     T1,     T1,    64
    addi.d     II,     II,    64
    addi.d     I,      I,     1
    blt        I,      T0,    .L02

    //Acc U2
    GACC   xvf, d, U4, U2
    xvreplve0.d U2, U4

.L03:  /* &4 */
    andi      T0,     J,   4
    beq       $r0,    T0,    .L04

    mul.d     T1,     J,    LDA
    add.d     T1,     T1,   II

    xvldx       U1,    AO1,    T1

    move        T1,    IY
    add.d       T2,    T1,    INCY
    add.d       T3,    T2,    INCY
    add.d       T4,    T3,    INCY

    fldx.d      $f4,   Y,     T1
    fldx.d      $f5,   Y,     T2
    fldx.d      $f6,   Y,     T3
    fldx.d      $f7,   Y,     T4

    vextrins.d  $vr4,    $vr5,    0x10
    vextrins.d  $vr6,    $vr7,    0x10
    xvpermi.q   U4,    U6,    0x02

    xvfmadd.d    U4,    U3,    U1,    U4

    xvpermi.d   U6,    U4,    0xee
    vextrins.d  $vr5,    $vr4,    0x01
    vextrins.d  $vr7,    $vr6,    0x01

    fstx.d      $f4,   Y,     T1
    fstx.d      $f5,   Y,     T2
    fstx.d      $f6,   Y,     T3
    fstx.d      $f7,   Y,     T4

    slli.d      T1,    INCY,  2
    add.d       IY,    IY,    T1

    move        T1,    IX
    add.d       T2,    T1,    INCX
    add.d       T3,    T2,    INCX
    add.d       T4,    T3,    INCX

    fldx.d      $f4,   X,     T1
    fldx.d      $f5,   X,     T2
    fldx.d      $f6,   X,     T3
    fldx.d      $f7,   X,     T4

    vextrins.d  $vr4,    $vr5,    0x10
    vextrins.d  $vr6,    $vr7,    0x10
    xvpermi.q   U4,    U6,    0x02

    xvand.v     $xr12, $xr2, $xr2

    xvfmadd.d    U2,    U1,    U4,    U2
    xvfsub.d     U2,    U2,    $xr12

    xvpermi.d    U4,    U2,    0x01
    xvpermi.d    U5,    U2,    0x02
    xvpermi.d    U6,    U2,    0x03

    fadd.d     $f2,    $f2,   $f4
    fadd.d     $f2,    $f2,   $f5
    fadd.d     $f2,    $f2,   $f6
    fadd.d     $f2,    $f2,   $f12

    xvreplve0.d U2,    U2

    slli.d     T2,    INCX,  2
    add.d      IX,    IX,    T2

    addi.d     II,    II,    32

.L04: /* &2 */
    andi      T0,     J,   2
    beq       $r0,    T0,    .L05

    mul.d     T1,     J,    LDA
    add.d     T1,     T1,   II

    vldx       $vr1,    AO1,    T1

    move        T1,    IY
    add.d       T2,    T1,    INCY

    fldx.d      $f6,   Y,     T1
    fldx.d      $f7,   Y,     T2

    vextrins.d  $vr6,    $vr7,    0x10
    vfmadd.d    $vr6,    $vr3,    $vr1,    $vr6
    vextrins.d  $vr7,    $vr6,    0x01

    fstx.d      $f6,   Y,     T1
    fstx.d      $f7,   Y,     T2

    slli.d      T1,    INCY,  1
    add.d       IY,    IY,    T1

    move        T1,    IX
    add.d       T2,    T1,    INCX

    fldx.d      $f6,   X,     T1
    fldx.d      $f7,   X,     T2

    vextrins.d  $vr6,    $vr7,    0x10
    vand.v      $vr12,   $vr2,    $vr2

    vfmadd.d    $vr2,    $vr1,    $vr6,    $vr2
    vfsub.d     $vr2,    $vr2,    $vr12

    vextrins.d  $vr4,    $vr2,    0x01
    fadd.d     $f2,    $f2,   $f4
    fadd.d     $f2,    $f2,   $f12

    xvreplve0.d U2,    U2

    slli.d     T2,    INCX,  1
    add.d      IX,    IX,    T2

    addi.d     II,    II,    16

.L05:  /* &1 */
    andi      T0,     J,   1
    beq       $r0,    T0,    .L06

    mul.d     T1,     J,    LDA
    add.d     T1,     T1,   II

    fldx.d      $f4,   AO1,   T1
    fldx.d      $f6,   Y,     IY
    fmadd.d     $f6,   $f3,   $f4,   $f6
    fstx.d      $f6,   Y,     IY
    add.d       IY,    IY,    INCY

    fldx.d      $f6,   X,     IX
    fmadd.d     $f2,   $f4,   $f6,   $f2
    add.d       IX,    IX,    INCX

    addi.d     II,    II,    8

.L06:
    mul.d     T1,     J,    LDA
    slli.d    T2,     J,    BASE_SHIFT
    add.d     T1,     T1,   T2

    fldx.d      $f6,   Y,     JY
    fldx.d      $f4,   AO1,   T1
    fmadd.d     $f6,   $f3,   $f4,   $f6
    fmul.d      $f7,   ALPHA, $f2
    fadd.d      $f6,   $f6,   $f7

    fstx.d      $f6,   Y,     JY

    add.d       JX,    JX,    INCX
    add.d       JY,    JY,    INCY

    addi.d      J,    J,    1
    blt         J,    M,    .L01

.L999:
    LDARG  $r23,  $sp,    0
    LDARG  $r24,  $sp,    8
    LDARG  $r25,  $sp,    16
    LDARG  $r26,  $sp,    32
    LDARG  $r27,  $sp,    40
    LDARG  $r28,  $sp,    48
    LDARG  $r29,  $sp,    56
    LDARG  $r30,  $sp,    64
    LDARG  $r31,  $sp,    72

    addi.d  $sp, $sp, 88
    jirl    $r0, $r1, 0x0

    EPILOGUE
