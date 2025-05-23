//===--- Assertions.cpp - Assertion macros --------------------------------===//
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
//
// This file defines implementation details of include/swift/Basic/Assertions.h.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "language/Basic/Assertions.h"
#include <iostream>

llvm::cl::opt<bool> AssertContinue(
    "assert-continue", llvm::cl::init(false),
    llvm::cl::desc("Do not stop on an assertion failure"));

llvm::cl::opt<bool> AssertHelp(
    "assert-help", llvm::cl::init(false),
    llvm::cl::desc("Print help for managing assertions"));

int CONDITIONAL_ASSERT_Global_enable_flag =
#ifdef NDEBUG
  0; // Default to `off` in release builds
#else
  1; // Default to `on` in debug builds
#endif

void ASSERT_failure(const char *expr, const char *filename, int line, const char *func) {
  // Find the last component of `filename`
  // Needed on Windows MSVC, which lacks __FILE_NAME__
  // so we have to use __FILE__ instead:
  for (const char *p = filename; *p != '\0'; p++) {
    if ((p[0] == '/' || p[0] == '\\')
       && p[1] != '/' && p[1] != '\\' && p[1] != '\0') {
      filename = p + 1;
    }
  }

  llvm::errs()
    << "Assertion failed: "
    << "(" << expr << "), "
    << "function " << func << " at "
    << filename << ":"
    << line << ".\n";

  ASSERT_help();

  if (AssertContinue) {
    llvm::errs() << "Continuing after failed assertion (-Xllvm -assert-continue)\n";
    return;
  }

  abort();
}

void ASSERT_help() {
  static int ASSERT_help_shown = 0;
  if (ASSERT_help_shown) {
    return;
  }
  ASSERT_help_shown = 1;

  if (!AssertHelp) {
    llvm::errs() << "(to display assertion configuration options: -Xllvm -assert-help)\n";
    return;
  }

  llvm::errs() << "\n";
  llvm::errs() << "Control assertion behavior with one or more of the following options:\n\n";
  llvm::errs() << " -Xllvm -assert-continue\n";
  llvm::errs() << "     Continue after any failed assertion\n\n";
}

// This has to be callable in the same way as the macro version,
// so we can't put it inside a namespace.
#undef CONDITIONAL_ASSERT_enabled
int CONDITIONAL_ASSERT_enabled() {
  return (CONDITIONAL_ASSERT_Global_enable_flag != 0);
}
