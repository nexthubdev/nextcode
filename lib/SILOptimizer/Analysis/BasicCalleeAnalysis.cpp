//===--- BasicCalleeAnalysis.cpp - Determine callees per call site --------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "language/SILOptimizer/Analysis/BasicCalleeAnalysis.h"

#include "language/AST/Decl.h"
#include "language/AST/ProtocolConformance.h"
#include "language/Basic/Statistic.h"
#include "language/SIL/MemAccessUtils.h"
#include "language/SIL/SILBridging.h"
#include "language/SIL/SILModule.h"
#include "language/SIL/Test.h"
#include "language/SILOptimizer/OptimizerBridging.h"
#include "language/SILOptimizer/PassManager/Transforms.h"
#include "language/SILOptimizer/Utils/InstOptUtils.h"
#include "llvm/Support/Compiler.h"

#include <algorithm>

#define DEBUG_TYPE "BasicCalleeAnalysis"

using namespace language;

void BasicCalleeAnalysis::dump() const {
  print(llvm::errs());
}

void BasicCalleeAnalysis::print(llvm::raw_ostream &os) const {
  if (!Cache) {
    os << "<no cache>\n";
  }
  llvm::DenseSet<SILDeclRef> printed;
  for (auto &VTable : M.getVTables()) {
    for (const SILVTable::Entry &entry : VTable->getEntries()) {
      if (printed.insert(entry.getMethod()).second) {
        os << "callees for " << entry.getMethod() << ":\n";
        Cache->getCalleeList(entry.getMethod()).print(os);
      }
    }
  }
}

//===----------------------------------------------------------------------===//
//                            Swift Bridging
//===----------------------------------------------------------------------===//

static BridgedCalleeAnalysis::IsDeinitBarrierFn instructionIsDeinitBarrierFunction;
static BridgedCalleeAnalysis::GetMemBehvaiorFn getMemBehvaiorFunction = nullptr;

void BridgedCalleeAnalysis::registerAnalysis(IsDeinitBarrierFn instructionIsDeinitBarrierFn,
                                             GetMemBehvaiorFn getMemBehvaiorFn) {
  instructionIsDeinitBarrierFunction = instructionIsDeinitBarrierFn;
  getMemBehvaiorFunction = getMemBehvaiorFn;
}

MemoryBehavior BasicCalleeAnalysis::
getMemoryBehavior(FullApplySite as, bool observeRetains) {
  if (getMemBehvaiorFunction) {
    auto b = getMemBehvaiorFunction({as.getInstruction()->asSILNode()},
                                    observeRetains,
                                    {this});
    return (MemoryBehavior)b;
  }
  return MemoryBehavior::MayHaveSideEffects;
}

bool swift::isDeinitBarrier(SILInstruction *const instruction,
                            BasicCalleeAnalysis *bca) {
  if (!instructionIsDeinitBarrierFunction || !bca) {
    return mayBeDeinitBarrierNotConsideringSideEffects(instruction);
  }
  BridgedInstruction inst = {
      cast<SILNode>(const_cast<SILInstruction *>(instruction))};
  BridgedCalleeAnalysis analysis = {bca};
  return instructionIsDeinitBarrierFunction(inst, analysis);
}

namespace language::test {
// Arguments:
// - instruction
// Dumps:
// - instruction
// - whether it's a deinit barrier
static FunctionTest IsDeinitBarrierTest("is_deinit_barrier", [](auto &function,
                                                                auto &arguments,
                                                                auto &test) {
  auto *instruction = arguments.takeInstruction();
  auto *analysis = test.template getAnalysis<BasicCalleeAnalysis>();
  auto isBarrier = isDeinitBarrier(instruction, analysis);
  instruction->print(llvm::outs());
  auto *boolString = isBarrier ? "true" : "false";
  llvm::outs() << boolString << "\n";
});
} // namespace language::test
