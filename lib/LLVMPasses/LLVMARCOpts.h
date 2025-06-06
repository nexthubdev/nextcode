//===--- LLVMARCOpts.h - LLVM level ARC Opts Util. Declarations -*- C++ -*-===//
//
// Copyright (c) NeXTHub Corporation. All rights reserved.
// DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
//
// This code is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// version 2 for more details (a copy is included in the LICENSE file that
// accompanied this code).
//
// Author(-s): Tunjay Akbarli
//
//===----------------------------------------------------------------------===//
#ifndef SWIFT_LLVMPASSES_LLVMARCOPTS_H
#define SWIFT_LLVMPASSES_LLVMARCOPTS_H

#include "language/Basic/LLVM.h"
#include "language/Runtime/Config.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"

namespace language {

enum RT_Kind {
#define KIND(Name, MemBehavior) RT_ ## Name,
#include "LLVMSwift.def"
};

/// Take a look at the specified instruction and classify it into what kind of
/// runtime entrypoint it is, if any.
inline RT_Kind classifyInstruction(const llvm::Instruction &I) {
  if (!I.mayReadOrWriteMemory())
    return RT_NoMemoryAccessed;

  // Non-calls or calls to indirect functions are unknown.
  auto *CI = dyn_cast<llvm::CallInst>(&I);
  if (CI == 0) return RT_Unknown;

  // First check if we have an objc intrinsic.
  auto intrinsic = CI->getIntrinsicID();
  switch (intrinsic) {
  // This is an intrinsic that we do not understand. It can not be one of our
  // "special" runtime functions as well... so return RT_Unknown early.
  default:
    return RT_Unknown;
  case llvm::Intrinsic::not_intrinsic:
    // If we do not have an intrinsic, break and move onto runtime functions
    // that we identify by name.
    break;
#define OBJC_FUNC(Name, MemBehavior, TextualName)                              \
  case llvm::Intrinsic::objc_##TextualName:                                    \
    return RT_##Name;
#include "LLVMSwift.def"
  }

  llvm::Function *F = CI->getCalledFunction();
  if (F == nullptr)
    return RT_Unknown;

  return llvm::StringSwitch<RT_Kind>(F->getName())
#define SWIFT_FUNC(Name, MemBehavior, TextualName) \
    .Case("swift_" #TextualName, RT_ ## Name)
#define SWIFT_INTERNAL_FUNC_NEVER_NONATOMIC(Name, MemBehavior, TextualName) \
    .Case("__swift_" #TextualName, RT_ ## Name)
#include "LLVMSwift.def"

    // Support non-atomic versions of reference counting entry points.
#define SWIFT_FUNC(Name, MemBehavior, TextualName) \
    .Case("swift_nonatomic_" #TextualName, RT_ ## Name)
#define OBJC_FUNC(Name, MemBehavior, TextualName) \
    .Case("objc_nonatomic_" #TextualName, RT_ ## Name)
#define SWIFT_INTERNAL_FUNC_NEVER_NONATOMIC(Name, MemBehavior, TextualName)
#include "LLVMSwift.def"

    .Default(RT_Unknown);
}

} // end namespace language
#endif
