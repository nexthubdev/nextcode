/***************************************************************************
Copyright (c) 2013-2017, The OpenBLAS Project
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
*****************************************************************************/

/**************************************************************************************
* 2017/01/01 AbdelRauf (quickwritereader@gmail.com)
* 	 BLASTEST 		: OK
* 	 CTEST			: OK
* 	 TEST			: OK
**************************************************************************************/

/*********************************************************************/
/* Copyright 2009, 2010 The University of Texas at Austin.           */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#define ASSEMBLER
#include "common.h"

/* 
 
#BLASLONG bm,BLASLONG bn,BLASLONG bk,FLOAT alpha,FLOAT* ba,FLOAT* bb,FLOAT* C,BLASLONG ldc
 ##bm=r2,bn=r3, bk=r4, alpha=f0,ba=r5,bb=r6,stack[160] ,ldc=stack[168]  
**********************************************************************************************/
/*Note: r0 can not be used as address disp register */  

#define BM           %r2
#define BM_CUR       %r0
#define BN           %r3
#define BN_CUR       %r10 
#define BK           %r4 
#define LDC_BYTE     %r8 
#define   ALPHA      %f0
#define ALPHA_VECT   %v0 
#define  LOCAL_VAR1  %r9
#define LOCAL_VAR2   %r1
#define LOCAL_VAR3   %r11
#define A            %r5
#define B            %r6
#define CIJ          %r7
#define CIJ_LOCAL    %r12
#define ALIGN_4   .align 16
#define ALIGN_2   .align 8
#define PREFETCH_INS 1

#include "kernelMacros.S"

/***********************************DGEMM***********************************************************/

PROLOGUE
 
stmg %r6,%r12,48(%r15)   
lg CIJ, 160(%r15)
lg LOCAL_VAR1, 168(%r15)
srlg BN_CUR,BN,2  
vrepg ALPHA_VECT,ALPHA_VECT,0 /*replicate alpha which in f0*/
sllg LDC_BYTE, LOCAL_VAR1,3  /*calculate lcd stride with bytes double=8 x<<3 */
cijle BN_CUR,0,.LX2 

ALIGN_4
.LX4_BN:
#if defined(PREFETCH_INS)
        pfd 1, 0(A)
        pfd 1, 256(A)   
        pfd 1, 0(B) 
        pfd 1, 256(B)  
#endif
srlg BM_CUR,BM,3  
lgr LOCAL_VAR3,A 
lgr CIJ_LOCAL,CIJ
cijle BM_CUR,0,.L4x4

ALIGN_4
.L8x4_BM: /*BM_CUR  LOOP */

srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_8x4  
cijle LOCAL_VAR1,0,.L8x4_mod

ALIGN_4
.L8x4_4_BK: /*BK_CUR LOOP */
#if defined(PREFETCH_INS)
        pfd 1, 512(LOCAL_VAR3)   
#endif
      CALC_8x4_4 LOCAL_VAR3,LOCAL_VAR2  
#if defined(PREFETCH_INS)
        pfd 1, 512(LOCAL_VAR2)  
#endif 
brctg LOCAL_VAR1,.L8x4_4_BK

ALIGN_4
.L8x4_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L8x4_BK_Store

ALIGN_4
.L8x4_BK: /*BK_CUR LOOP */
      CALC_8x4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L8x4_BK

ALIGN_4
.L8x4_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_8x4 ALPHA_VECT,CIJ_LOCAL, LDC_BYTE
 
brctg BM_CUR,.L8x4_BM 
 
ALIGN_4
.L4x4:
  
tmll BM,4   
jz .L2x4
 
ALIGN_4
.L4x4_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_4x4  
cijle LOCAL_VAR1,0,.L4x4_mod

ALIGN_4
.L4x4_4_BK: /*BK_CUR LOOP */
      CALC_4x4_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L4x4_4_BK

ALIGN_4
.L4x4_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L4x4_BK_Store

ALIGN_4
.L4x4_BK: /*BK_CUR LOOP */
      CALC_4x4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L4x4_BK

ALIGN_4
.L4x4_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_4x4 ALPHA_VECT ,CIJ_LOCAL, LDC_BYTE
 
ALIGN_2
.L2x4:
 
tmll BM,2  
jz .L1x4
 
ALIGN_4
.L2x4_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_2x4  
cijle LOCAL_VAR1,0,.L2x4_mod

ALIGN_4
.L2x4_4_BK: /*BK_CUR LOOP */
      CALC_2x4_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L2x4_4_BK

ALIGN_4
.L2x4_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L2x4_BK_Store

ALIGN_4
.L2x4_BK: /*BK_CUR LOOP */
      CALC_2x4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L2x4_BK

ALIGN_4
.L2x4_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_2x4 ALPHA_VECT ,CIJ_LOCAL, LDC_BYTE


ALIGN_4
.L1x4:
 
tmll BM,1  
jz .Lx4_INNER_END
 
ALIGN_4
.L1x4_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_1x4  
cijle LOCAL_VAR1,0,.L1x4_mod

ALIGN_4
.L1x4_4_BK: /*BK_CUR LOOP */
      CALC_1x4_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L1x4_4_BK

ALIGN_4
.L1x4_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L1x4_BK_Store

ALIGN_4
.L1x4_BK: /*BK_CUR LOOP */
      CALC_1x4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L1x4_BK

ALIGN_4
.L1x4_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_1x4 ALPHA_VECT ,CIJ_LOCAL, LDC_BYTE

ALIGN_2
.Lx4_INNER_END:

/*add LDC_BYTE_COPY to new*/
sllg LOCAL_VAR1,LDC_BYTE,2  /*multiply*4 */
sllg LOCAL_VAR2,BK,5  /*muyliply*4*sizeof(double)  =multiply*32* 2**5 */
la CIJ,0(CIJ,LOCAL_VAR1)  /*refresh CIJ=CIJ+LDC_BYTE*4*/ 
la B,0(B,LOCAL_VAR2)      /*refresh B=B+Bk*4*sizeof(double)  */

brctg BN_CUR,.LX4_BN

/*********************************X2 SECTION************************************************/
ALIGN_4
.LX2:
tmll BN,2 
jz .Lx1

ALIGN_4
.Lx2_BN:
srlg BM_CUR,BM,3  
lgr LOCAL_VAR3,A
lgr CIJ_LOCAL,CIJ
cijle BM_CUR,0,.L4x2


ALIGN_4
.L8x2_BM: /*BM_CUR  LOOP */
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_8x2  
cijle LOCAL_VAR1,0,.L8x2_mod

ALIGN_4
.L8x2_4_BK: /*BK_CUR LOOP */
#if defined(PREFETCH_INS)
        pfd 1, 256(LOCAL_VAR3)  
        pfd 1,64(LOCAL_VAR2) 
#endif
      CALC_8x2_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L8x2_4_BK

ALIGN_4
.L8x2_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L8x2_BK_Store

ALIGN_4
.L8x2_BK: /*BK_CUR LOOP */
      CALC_8x2 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L8x2_BK

ALIGN_4
.L8x2_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_8x2 ALPHA_VECT,CIJ_LOCAL, LDC_BYTE
 
ALIGN_4
brctg BM_CUR,.L8x2_BM 
 
ALIGN_2
.L4x2:
 
tmll BM,4 
jz .L2x2
 
ALIGN_4
.L4x2_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_4x2  
cijle LOCAL_VAR1,0,.L4x2_mod

ALIGN_4
.L4x2_4_BK: /*BK_CUR LOOP */
      CALC_4x2_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L4x2_4_BK

ALIGN_4
.L4x2_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L4x2_BK_Store

ALIGN_4
.L4x2_BK: /*BK_CUR LOOP */
      CALC_4x2 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L4x2_BK

ALIGN_4
.L4x2_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_4x2 ALPHA_VECT ,CIJ_LOCAL, LDC_BYTE
 
ALIGN_2
.L2x2:

tmll BM,2   
jz .L1x2
 
ALIGN_4
.L2x2_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_2x2  
cijle LOCAL_VAR1,0,.L2x2_mod

ALIGN_4
.L2x2_4_BK: /*BK_CUR LOOP */
      CALC_2x2_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L2x2_4_BK

ALIGN_4
.L2x2_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L2x2_BK_Store

ALIGN_4
.L2x2_BK: /*BK_CUR LOOP */
      CALC_2x2 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L2x2_BK

ALIGN_4
.L2x2_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_2x2 ALPHA_VECT ,CIJ_LOCAL, LDC_BYTE


ALIGN_2
.L1x2:
 
tmll BM,1  
jz .Lx2_INNER_END
 
ALIGN_4
.L1x2_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_1x2  
cijle LOCAL_VAR1,0,.L1x2_mod

ALIGN_4
.L1x2_4_BK: /*BK_CUR LOOP */
      CALC_1x2_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L1x2_4_BK

ALIGN_4
.L1x2_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L1x2_BK_Store

ALIGN_4
.L1x2_BK: /*BK_CUR LOOP */
      CALC_1x2 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L1x2_BK

ALIGN_4
.L1x2_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_1x2 ALPHA_VECT ,CIJ_LOCAL, LDC_BYTE

ALIGN_2
.Lx2_INNER_END: 
/*add LDC_BYTE_COPY to new*/
la LOCAL_VAR1,0(LDC_BYTE,LDC_BYTE)   /*multiply*2 */
sllg LOCAL_VAR2,BK,4  /*muyliply*2*sizeof(double)  =multiply*16* 2**4 */
la CIJ,0(CIJ,LOCAL_VAR1)  /*refresh CIJ=CIJ+LDC_BYTE*4*/ 
la B,0(B,LOCAL_VAR2)      /*refresh B=B+Bk*4*sizeof(double)  */

 
 

/*********************************X1 SECTION************************************************/
ALIGN_2
.Lx1:
tmll BN,1  
jz .L_FUNC_END

ALIGN_4
.Lx1_BN:
srlg BM_CUR,BM,3  
lgr LOCAL_VAR3,A
lgr CIJ_LOCAL,CIJ
cijle BM_CUR,0,.L4x1


ALIGN_4
.L8x1_BM: /*BM_CUR  LOOP */
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_8x1  
cijle LOCAL_VAR1,0,.L8x1_mod

ALIGN_4
.L8x1_4_BK: /*BK_CUR LOOP */
#if defined(PREFETCH_INS)
        pfd 1, 256(LOCAL_VAR3)
#endif  
      CALC_8x1_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L8x1_4_BK

ALIGN_4
.L8x1_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L8x1_BK_Store

ALIGN_4
.L8x1_BK: /*BK_CUR LOOP */
      CALC_8x1 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L8x1_BK

ALIGN_4
.L8x1_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_8x1 ALPHA_VECT,CIJ_LOCAL, LDC_BYTE
 
ALIGN_4
brctg BM_CUR,.L8x1_BM 
 
ALIGN_2
.L4x1:
 
tmll BM,4  
jz .L2x1
 
ALIGN_4
.L4x1_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_4x1  
cijle LOCAL_VAR1,0,.L4x1_mod

ALIGN_4
.L4x1_4_BK: /*BK_CUR LOOP */
      CALC_4x1_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L4x1_4_BK

ALIGN_4
.L4x1_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L4x1_BK_Store

ALIGN_4
.L4x1_BK: /*BK_CUR LOOP */
      CALC_4x1 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L4x1_BK

ALIGN_4
.L4x1_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_4x1 ALPHA_VECT ,CIJ_LOCAL, LDC_BYTE
 
ALIGN_2
.L2x1:

tmll BM,2   
jz .L1x1
 
ALIGN_4
.L2x1_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_2x1  
cijle LOCAL_VAR1,0,.L2x1_mod

ALIGN_4
.L2x1_4_BK: /*BK_CUR LOOP */
      CALC_2x1_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L2x1_4_BK

ALIGN_4
.L2x1_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L2x1_BK_Store

ALIGN_4
.L2x1_BK: /*BK_CUR LOOP */
      CALC_2x1 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L2x1_BK

ALIGN_4
.L2x1_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_2x1 ALPHA_VECT ,CIJ_LOCAL, LDC_BYTE


ALIGN_2
.L1x1:
 
tmll BM, 1   
jz .Lx1_INNER_END
 
ALIGN_4
.L1x1_BM: /*BM start*/
srlg LOCAL_VAR1,BK,2  /*refresh BK*/  
lgr LOCAL_VAR2,B      /*refresh BPOINT*/
ZERO_CVEC_1x1  
cijle LOCAL_VAR1,0,.L1x1_mod

ALIGN_4
.L1x1_4_BK: /*BK_CUR LOOP */
      CALC_1x1_4 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L1x1_4_BK

ALIGN_4
.L1x1_mod:
lghi LOCAL_VAR1,3
NGR LOCAL_VAR1,BK /*refresh BK*/ 
jz .L1x1_BK_Store

ALIGN_4
.L1x1_BK: /*BK_CUR LOOP */
      CALC_1x1 LOCAL_VAR3,LOCAL_VAR2   
brctg LOCAL_VAR1,.L1x1_BK

ALIGN_4
.L1x1_BK_Store:
/*store C and use LDC_BYTE  AND CIJ_COPY for mem storing*/
STORE_1x1 ALPHA ,CIJ_LOCAL, LDC_BYTE

ALIGN_2
.Lx1_INNER_END: 
/*add LDC_BYTE_COPY to new*/ 
sllg LOCAL_VAR2,BK,3  /*muyliply*2*sizeof(double)  =multiply*8* 2**3 */
la CIJ,0(CIJ,LDC_BYTE)  /*refresh CIJ=CIJ+LDC_BYTE */ 
la B,0(B,LOCAL_VAR2)      /*refresh B=B+Bk*1*sizeof(double)  */
 

ALIGN_2
.L_FUNC_END:
/*end*/
lmg %r6,%r12,48(%r15) 
br %r14

EPILOGUE
.end




