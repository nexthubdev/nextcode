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

#include <stdio.h>
#include "common.h"
#ifdef FUNCTION_PROFILE
#include "functable.h"
#endif

#ifndef COMPLEX
#ifdef XDOUBLE
#define ERROR_NAME "QGESV"
#elif defined(DOUBLE)
#define ERROR_NAME "DGESV"
#else
#define ERROR_NAME "SGESV"
#endif
#else
#ifdef XDOUBLE
#define ERROR_NAME "XGESV"
#elif defined(DOUBLE)
#define ERROR_NAME "ZGESV"
#else
#define ERROR_NAME "CGESV"
#endif
#endif

int NAME(blasint *N, blasint *NRHS, FLOAT *a, blasint *ldA, blasint *ipiv,
         FLOAT *b, blasint *ldB, blasint *Info){

  blas_arg_t args;

  blasint info;
  FLOAT *buffer;
#ifdef PPC440
  extern
#endif
  FLOAT *sa, *sb;

  PRINT_DEBUG_NAME;

  args.m    = *N;
  args.n    = *NRHS;
  args.a    = (void *)a;
  args.lda  = *ldA;
  args.b    = (void *)b;
  args.ldb  = *ldB;
  args.c    = (void *)ipiv;

  info  = 0;
  if (args.ldb < MAX(1,args.m)) info = 7;
  if (args.lda < MAX(1,args.m)) info = 4;
  if (args.n   < 0)             info = 2;
  if (args.m   < 0)             info = 1;

  if (info) {
    BLASFUNC(xerbla)(ERROR_NAME, &info, sizeof(ERROR_NAME) - 1);
    *Info = - info;
    return 0;
  }

  args.alpha = NULL;
  args.beta  = NULL;

  *Info = 0;

  if (args.m == 0 || args.n == 0) return 0;

  IDEBUG_START;

  FUNCTION_PROFILE_START();

#ifndef PPC440
  buffer = (FLOAT *)blas_memory_alloc(1);
  
  sa = (FLOAT *)((BLASLONG)buffer + GEMM_OFFSET_A);
  sb = (FLOAT *)(((BLASLONG)sa + ((GEMM_P * GEMM_Q * COMPSIZE * SIZE + GEMM_ALIGN) & ~GEMM_ALIGN)) + GEMM_OFFSET_B);
#endif

#ifdef SMP
  args.common = NULL;

#if defined(_WIN64) && defined(_M_ARM64)
  #ifdef COMPLEX
    if (args.m * args.n <= 300) 
  #else
    if (args.m * args.n <= 500) 
  #endif
      args.nthreads = 1;
  else if (args.m * args.n <= 1000)
    args.nthreads = 4;
  else
    args.nthreads = num_cpu_avail(4);
#else
  #ifndef DOUBLE
    if (args.m * args.n < 40000)
  #else
    if (args.m * args.n < 10000)
  #endif
      args.nthreads = 1;
    else
      args.nthreads = num_cpu_avail(4);
#endif

  if (args.nthreads == 1) {
#endif

    args.n    = *N;
    info = GETRF_SINGLE(&args, NULL, NULL, sa, sb, 0);

    if (info == 0){
      args.n    = *NRHS;
      GETRS_N_SINGLE(&args, NULL, NULL, sa, sb, 0);
    }

#ifdef SMP
  } else {

    args.n    = *N;
    info = GETRF_PARALLEL(&args, NULL, NULL, sa, sb, 0);

    if (info == 0){
      args.n    = *NRHS;
      GETRS_N_PARALLEL(&args, NULL, NULL, sa, sb, 0);
    }
  }
#endif

#ifndef PPC440
  blas_memory_free(buffer);
#endif

  *Info = info;

  FUNCTION_PROFILE_END(COMPSIZE * COMPSIZE, *N * *N, 2. / 3. * *N * *N * *N + *N * *N);

  IDEBUG_END;

  return 0;
}
