//===--- modulewrap_main.cpp - module wrapping utility --------------------===//
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
// Wraps .swiftmodule files inside an object file container so they
// can be passed to the linker directly. Mostly useful for platforms
// where the debug info typically stays in the executable.
// (ie. ELF-based platforms).
//
//===----------------------------------------------------------------------===//

#include "language/AST/DiagnosticsFrontend.h"
#include "language/Basic/LLVMInitialize.h"
#include "language/Frontend/Frontend.h"
#include "language/Frontend/PrintingDiagnosticConsumer.h"
#include "language/Option/Options.h"
#include "language/Serialization/Validation.h"
#include "language/SIL/SILModule.h"
#include "language/SIL/TypeLowering.h"
#include "language/Subsystems.h"
#include "language/SymbolGraphGen/SymbolGraphOptions.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Bitstream/BitstreamReader.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/Option.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/VirtualOutputBackends.h"

using namespace llvm::opt;
using namespace language;

class ModuleWrapInvocation {
private:
  std::string MainExecutablePath;
  std::string OutputFilename = "-";
  llvm::Triple TargetTriple;
  std::vector<std::string> InputFilenames;
  bool UseSharedResourceFolder = true;
  bool EnableObjCInterop = true;

public:
  bool hasSingleInput() const { return InputFilenames.size() == 1; }
  const std::string &getFilenameOfFirstInput() const {
    return InputFilenames[0];
  }

  void setMainExecutablePath(const std::string &Path) {
    MainExecutablePath = Path;
  }

  const std::string &getOutputFilename() { return OutputFilename; }

  const std::vector<std::string> &getInputFilenames() { return InputFilenames; }
  llvm::Triple &getTargetTriple() { return TargetTriple; }

  bool useSharedResourceFolder() { return UseSharedResourceFolder; }
  bool enableObjCInterop() { return EnableObjCInterop; }

  int parseArgs(llvm::ArrayRef<const char *> Args, DiagnosticEngine &Diags) {
    using namespace options;

    // Parse frontend command line options using Swift's option table.
    std::unique_ptr<llvm::opt::OptTable> Table = createSwiftOptTable();
    unsigned MissingIndex;
    unsigned MissingCount;
    llvm::opt::InputArgList ParsedArgs =
      Table->ParseArgs(Args, MissingIndex, MissingCount,
                       ModuleWrapOption);
    if (MissingCount) {
      Diags.diagnose(SourceLoc(), diag::error_missing_arg_value,
                     ParsedArgs.getArgString(MissingIndex), MissingCount);
      return 1;
    }

    if (const Arg *A = ParsedArgs.getLastArg(options::OPT_target))
      TargetTriple = llvm::Triple(llvm::Triple::normalize(A->getValue()));
    else
      TargetTriple = llvm::Triple(llvm::sys::getDefaultTargetTriple());

    if (ParsedArgs.hasArg(OPT_UNKNOWN)) {
      for (const Arg *A : ParsedArgs.filtered(OPT_UNKNOWN)) {
        Diags.diagnose(SourceLoc(), diag::error_unknown_arg,
                       A->getAsString(ParsedArgs));
      }
      return true;
    }

    if (ParsedArgs.getLastArg(OPT_help)) {
      std::string ExecutableName =
          llvm::sys::path::stem(MainExecutablePath).str();
      Table->printHelp(llvm::outs(), ExecutableName.c_str(),
                       "Swift Module Wrapper", options::ModuleWrapOption, 0,
                       /*ShowAllAliases*/false);
      return 1;
    }

    for (const Arg *A : ParsedArgs.filtered(OPT_INPUT)) {
      InputFilenames.push_back(A->getValue());
    }

    if (InputFilenames.empty()) {
      Diags.diagnose(SourceLoc(), diag::error_mode_requires_an_input_file);
      return 1;
    }

    if (const Arg *A = ParsedArgs.getLastArg(OPT_o)) {
      OutputFilename = A->getValue();
    }

    if (ParsedArgs.hasFlag(OPT_static_executable, OPT_no_static_executable,
                           false) ||
        ParsedArgs.hasFlag(OPT_static_stdlib, OPT_no_static_stdlib, false) ||
        ParsedArgs.hasArg(OPT_static)) {
      UseSharedResourceFolder = false;
    }

    EnableObjCInterop = ParsedArgs.hasFlag(OPT_enable_objc_interop,
        OPT_disable_objc_interop, TargetTriple.isOSDarwin());

    return 0;
  }
};

int modulewrap_main(ArrayRef<const char *> Args, const char *Argv0,
                    void *MainAddr) {
  INITIALIZE_LLVM();

  CompilerInstance Instance;
  PrintingDiagnosticConsumer PDC;
  Instance.addDiagnosticConsumer(&PDC);

  ModuleWrapInvocation Invocation;
  std::string MainExecutablePath =
      llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
  Invocation.setMainExecutablePath(MainExecutablePath);

  // Parse arguments.
  if (Invocation.parseArgs(Args, Instance.getDiags()) != 0) {
    return 1;
  }

  if (!Invocation.hasSingleInput()) {
    Instance.getDiags().diagnose(SourceLoc(),
                                 diag::error_mode_requires_one_input_file);
    return 1;
  }

  StringRef Filename = Invocation.getFilenameOfFirstInput();
  auto ErrOrBuf = llvm::MemoryBuffer::getFile(Filename);
  if (!ErrOrBuf) {
    Instance.getDiags().diagnose(
        SourceLoc(), diag::error_no_such_file_or_directory, Filename);
    return 1;
  }

  // Superficially verify that the input is a swift module file.
  if (!serialization::isSerializedAST(ErrOrBuf.get()->getBuffer())) {
    Instance.getDiags().diagnose(SourceLoc(), diag::error_parse_input_file,
                                 Filename, "signature mismatch");
    return 1;
  }

  // Wrap the bitstream in a module object file. To use the ClangImporter to
  // create the module loader, we need to properly set the runtime library path.
  SearchPathOptions SearchPathOpts;
  SmallString<128> RuntimeResourcePath;
  CompilerInvocation::computeRuntimeResourcePathFromExecutablePath(
      MainExecutablePath, Invocation.useSharedResourceFolder(),
      RuntimeResourcePath);
  SearchPathOpts.RuntimeResourcePath = std::string(RuntimeResourcePath.str());

  SourceManager SrcMgr;
  SILOptions SILOpts;
  TypeCheckerOptions TypeCheckOpts;
  LangOptions LangOpts;
  ClangImporterOptions ClangImporterOpts;
  symbolgraphgen::SymbolGraphOptions SymbolGraphOpts;
  CASOptions CASOpts;
  SerializationOptions SerializationOpts;
  LangOpts.Target = Invocation.getTargetTriple();
  LangOpts.EnableObjCInterop = Invocation.enableObjCInterop();
  ASTContext &ASTCtx = *ASTContext::get(
      LangOpts, TypeCheckOpts, SILOpts, SearchPathOpts, ClangImporterOpts,
      SymbolGraphOpts, CASOpts, SerializationOpts, SrcMgr, Instance.getDiags(),
      llvm::makeIntrusiveRefCnt<llvm::vfs::OnDiskOutputBackend>());
  registerParseRequestFunctions(ASTCtx.evaluator);
  registerTypeCheckerRequestFunctions(ASTCtx.evaluator);

  ASTCtx.addModuleLoader(ClangImporter::create(ASTCtx, "",
                                               nullptr, nullptr,
                                               true),
                         true);
  ModuleDecl *M =
      ModuleDecl::createEmpty(ASTCtx.getIdentifier("swiftmodule"), ASTCtx);
  std::unique_ptr<Lowering::TypeConverter> TC(
      new Lowering::TypeConverter(*M, ASTCtx.SILOpts.EnableSILOpaqueValues));
  std::unique_ptr<SILModule> SM = SILModule::createEmptyModule(M, *TC, SILOpts);
  createSwiftModuleObjectFile(*SM, (*ErrOrBuf)->getBuffer(),
                              Invocation.getOutputFilename());
  return 0;
}
