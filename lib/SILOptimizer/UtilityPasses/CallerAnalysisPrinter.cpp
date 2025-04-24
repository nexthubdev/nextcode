//===--- CallerAnalysisPrinter.cpp - Caller Analysis test pass ------------===//
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
//
// This pass prints all the callsites of every function in the module.
//
//===----------------------------------------------------------------------===//

#include "language/SIL/SILFunction.h"
#include "language/SIL/SILModule.h"
#include "language/SILOptimizer/Analysis/CallerAnalysis.h"
#include "language/SILOptimizer/PassManager/Transforms.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"

using namespace language;

#define DEBUG_TYPE "caller-analysis-printer"

namespace {

/// A pass that dumps the caller analysis state in yaml form. Intended to allow
/// for visualizing of the caller analysis via external data visualization and
/// analysis programs.
class CallerAnalysisPrinterPass : public SILModuleTransform {
  /// The entry point to the transformation.
  void run() override {
    auto *CA = getAnalysis<CallerAnalysis>();
    CA->print(llvm::outs());
  }
};

} // end anonymous namespace

SILTransform *swift::createCallerAnalysisPrinter() {
  return new CallerAnalysisPrinterPass();
}
