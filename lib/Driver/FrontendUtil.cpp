//===--- FrontendUtil.cpp - Driver Utilities for Frontend -----------------===//
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

#include "language/Driver/FrontendUtil.h"

#include "language/AST/DiagnosticsDriver.h"
#include "language/Driver/Action.h"
#include "language/Driver/Compilation.h"
#include "language/Driver/Driver.h"
#include "language/Driver/Job.h"
#include "language/Driver/ToolChain.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/TargetParser/Host.h"

using namespace language;
using namespace language::driver;

void swift::driver::ExpandResponseFilesWithRetry(llvm::StringSaver &Saver,
                                llvm::SmallVectorImpl<const char *> &Args) {
  const unsigned MAX_COUNT = 30;
  for (unsigned I = 0; I != MAX_COUNT; ++I) {
    if (llvm::cl::ExpandResponseFiles(Saver,
        llvm::Triple(llvm::sys::getProcessTriple()).isOSWindows()
          ? llvm::cl::TokenizeWindowsCommandLine
          : llvm::cl::TokenizeGNUCommandLine,
        Args)) {
      return;
    }
  }
}

static void removeSupplementaryOutputs(llvm::opt::ArgList &ArgList) {
  llvm::DenseSet<unsigned> OptSpecifiersToRemove;

  for (llvm::opt::Arg *Arg : ArgList.getArgs()) {
    if (!Arg)
      continue;

    const llvm::opt::Option &Opt = Arg->getOption();
    if (Opt.hasFlag(options::SupplementaryOutput))
      OptSpecifiersToRemove.insert(Opt.getID());
  }

  for (unsigned Specifier : OptSpecifiersToRemove) {
    ArgList.eraseArg(Specifier);
  }
}

bool swift::driver::getSingleFrontendInvocationFromDriverArguments(
    StringRef DriverPath, ArrayRef<const char *> Argv, DiagnosticEngine &Diags,
    llvm::function_ref<bool(ArrayRef<const char *> FrontendArgs)> Action,
    bool ForceNoOutputs) {
  SmallVector<const char *, 16> Args;
  Args.push_back("<swiftc>"); // FIXME: Remove dummy argument.
  Args.insert(Args.end(), Argv.begin(), Argv.end());

  // When creating a CompilerInvocation, ensure that the driver creates a single
  // frontend command.
  Args.push_back("-whole-module-optimization");

  // Explicitly disable batch mode to avoid a spurious warning when combining
  // -enable-batch-mode with -whole-module-optimization.  This is an
  // implementation detail.
  Args.push_back("-disable-batch-mode");

  // Avoid using filelists
  std::string neverThreshold =
      std::to_string(Compilation::NEVER_USE_FILELIST);
  Args.push_back("-driver-filelist-threshold");
  Args.push_back(neverThreshold.c_str());

  // Expand any file list args.
  llvm::BumpPtrAllocator Allocator;
  llvm::StringSaver Saver(Allocator);
  ExpandResponseFilesWithRetry(Saver, Args);

  // Force the driver into batch mode by specifying "swiftc" as the name.
  Driver TheDriver(DriverPath, "swiftc", Args, Diags);

  // Don't check for the existence of input files, since the user of the
  // CompilerInvocation may wish to remap inputs to source buffers.
  TheDriver.setCheckInputFilesExist(false);

  TheDriver.setIsDummyDriverForFrontendInvocation(true);

  std::unique_ptr<llvm::opt::InputArgList> ArgList =
    TheDriver.parseArgStrings(ArrayRef<const char *>(Args).slice(1));
  if (Diags.hadAnyError())
    return true;

  if (ForceNoOutputs) {
    // Clear existing output modes and supplementary outputs.
    ArgList->eraseArg(options::OPT_modes_Group);
    removeSupplementaryOutputs(*ArgList);

    unsigned index = ArgList->MakeIndex("-typecheck");
    // Takes ownership of the Arg.
    ArgList->append(new llvm::opt::Arg(
        TheDriver.getOpts().getOption(options::OPT_typecheck),
        ArgList->getArgString(index), index));
  }

  std::unique_ptr<ToolChain> TC = TheDriver.buildToolChain(*ArgList);
  if (Diags.hadAnyError())
    return true;

  std::unique_ptr<Compilation> C =
      TheDriver.buildCompilation(*TC, std::move(ArgList), /*AllowErrors=*/true);
  if (!C || C->getJobs().empty())
    return true; // Don't emit an error; one should already have been emitted

  SmallPtrSet<const Job *, 4> CompileCommands;
  for (const Job *Cmd : C->getJobs()) {
    if (isa<CompileJobAction>(Cmd->getSource())) {
      CompileCommands.insert(Cmd);
    }
  }

  if (CompileCommands.size() != 1) {
    // TODO: include Jobs in the diagnostic.
    Diags.diagnose(SourceLoc(), diag::error_expected_one_frontend_job);
    return true;
  }

  const Job *Cmd = *CompileCommands.begin();
  if (StringRef("-frontend") != Cmd->getArguments().front()) {
    Diags.diagnose(SourceLoc(), diag::error_expected_frontend_command);
    return true;
  }

  const llvm::opt::ArgStringList &BaseFrontendArgs = Cmd->getArguments();
  return Action(llvm::ArrayRef(BaseFrontendArgs).drop_front());
}
