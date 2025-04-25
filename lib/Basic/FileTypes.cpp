//===--- FileTypes.cpp - Input & output formats used by the tools ---------===//
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

#include "language/Basic/Assertions.h"
#include "language/Basic/FileTypes.h"

#include "language/Strings.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

using namespace language;
using namespace language::file_types;

namespace {
struct TypeInfo {
  const char *Name;
  const char *Flags;
  const char *Extension;
};
} // end anonymous namespace

static const TypeInfo TypeInfos[] = {
#define TYPE(NAME, ID, EXTENSION, FLAGS) \
  { NAME, FLAGS, EXTENSION },
#include "language/Basic/FileTypes.def"
};

static const TypeInfo &getInfo(unsigned Id) {
  assert(Id >= 0 && Id < TY_INVALID && "Invalid Type ID.");
  return TypeInfos[Id];
}

StringRef file_types::getTypeName(ID Id) { return getInfo(Id).Name; }

StringRef file_types::getExtension(ID Id) {
  return getInfo(Id).Extension;
}

ID file_types::lookupTypeForExtension(StringRef Ext) {
  if (Ext.empty())
    return TY_INVALID;
  assert(Ext.front() == '.' && "not a file extension");
  return llvm::StringSwitch<file_types::ID>(Ext.drop_front())
#define TYPE(NAME, ID, EXTENSION, FLAGS) \
           .Case(EXTENSION, TY_##ID)
#include "language/Basic/FileTypes.def"
      .Default(TY_INVALID);
}

// Compute the file type from filename. This handles the lookup for extensions
// with multiple dots, like `.private.swiftinterface` correctly.
ID file_types::lookupTypeFromFilename(StringRef Filename) {
  StringRef MaybeExt = Filename;
  // Search from leftmost `.`, return the first match or till all dots are
  // consumed.
  size_t Pos = MaybeExt.find_first_of('.');
  while(Pos != StringRef::npos) {
    MaybeExt = MaybeExt.substr(Pos);
    // If size is 1, that means only `.` is left, return invalid.
    if (MaybeExt.size() == 1)
      return TY_INVALID;
    ID Type = lookupTypeForExtension(MaybeExt);
    if (Type != TY_INVALID)
      return Type;
    // Drop `.` and keep looking.
    MaybeExt = MaybeExt.drop_front();
    Pos = MaybeExt.find_first_of('.');
  }

  return TY_INVALID;
}

ID file_types::lookupTypeForName(StringRef Name) {
  return llvm::StringSwitch<file_types::ID>(Name)
#define TYPE(NAME, ID, EXTENSION, FLAGS) \
           .Case(NAME, TY_##ID)
#include "language/Basic/FileTypes.def"
      .Case("objc-header", TY_ClangHeader)
      .Default(TY_INVALID);
}

bool file_types::isTextual(ID Id) {
  switch (Id) {
  case file_types::TY_Swift:
  case file_types::TY_SIL:
  case file_types::TY_LoweredSIL:
  case file_types::TY_Dependencies:
  case file_types::TY_Assembly:
  case file_types::TY_ASTDump:
  case file_types::TY_RawSIL:
  case file_types::TY_RawLLVM_IR:
  case file_types::TY_LLVM_IR:
  case file_types::TY_ClangHeader:
  case file_types::TY_AutolinkFile:
  case file_types::TY_ImportedModules:
  case file_types::TY_TBD:
  case file_types::TY_ModuleTrace:
  case file_types::TY_FineModuleTrace:
  case file_types::TY_YAMLOptRecord:
  case file_types::TY_SwiftModuleInterfaceFile:
  case file_types::TY_PrivateSwiftModuleInterfaceFile:
  case file_types::TY_PackageSwiftModuleInterfaceFile:
  case file_types::TY_SwiftOverlayFile:
  case file_types::TY_JSONDependencies:
  case file_types::TY_JSONArguments:
  case file_types::TY_SwiftABIDescriptor:
  case file_types::TY_SwiftAPIDescriptor:
  case file_types::TY_ConstValues:
  case file_types::TY_SymbolGraphFile:
    return true;
  case file_types::TY_Image:
  case file_types::TY_Object:
  case file_types::TY_dSYM:
  case file_types::TY_PCH:
  case file_types::TY_SIB:
  case file_types::TY_RawSIB:
  case file_types::TY_SwiftModuleFile:
  case file_types::TY_SwiftModuleDocFile:
  case file_types::TY_SwiftSourceInfoFile:
  case file_types::TY_SwiftCrossImportDir:
  case file_types::TY_SwiftModuleSummaryFile:
  case file_types::TY_LLVM_BC:
  case file_types::TY_SerializedDiagnostics:
  case file_types::TY_ClangModuleFile:
  case file_types::TY_SwiftDeps:
  case file_types::TY_ExternalSwiftDeps:
  case file_types::TY_Nothing:
  case file_types::TY_Remapping:
  case file_types::TY_IndexData:
  case file_types::TY_BitstreamOptRecord:
  case file_types::TY_IndexUnitOutputPath:
  case file_types::TY_SwiftFixIt:
  case file_types::TY_ModuleSemanticInfo:
  case file_types::TY_CachedDiagnostics:
    return false;
  case file_types::TY_INVALID:
    llvm_unreachable("Invalid type ID.");
  }

  // Work around MSVC warning: not all control paths return a value
  llvm_unreachable("All switch cases are covered");
}

bool file_types::isAfterLLVM(ID Id) {
  switch (Id) {
  case file_types::TY_Assembly:
  case file_types::TY_LLVM_IR:
  case file_types::TY_LLVM_BC:
  case file_types::TY_Object:
    return true;
  case file_types::TY_Swift:
  case file_types::TY_PCH:
  case file_types::TY_ImportedModules:
  case file_types::TY_TBD:
  case file_types::TY_SIL:
  case file_types::TY_LoweredSIL:
  case file_types::TY_Dependencies:
  case file_types::TY_ASTDump:
  case file_types::TY_RawSIL:
  case file_types::TY_ClangHeader:
  case file_types::TY_AutolinkFile:
  case file_types::TY_Image:
  case file_types::TY_RawLLVM_IR:
  case file_types::TY_dSYM:
  case file_types::TY_SIB:
  case file_types::TY_RawSIB:
  case file_types::TY_SwiftModuleFile:
  case file_types::TY_SwiftModuleDocFile:
  case file_types::TY_SwiftSourceInfoFile:
  case file_types::TY_SwiftCrossImportDir:
  case file_types::TY_SwiftModuleSummaryFile:
  case file_types::TY_SwiftOverlayFile:
  case file_types::TY_SerializedDiagnostics:
  case file_types::TY_ClangModuleFile:
  case file_types::TY_SwiftDeps:
  case file_types::TY_ExternalSwiftDeps:
  case file_types::TY_Nothing:
  case file_types::TY_Remapping:
  case file_types::TY_IndexData:
  case file_types::TY_ModuleTrace:
  case file_types::TY_FineModuleTrace:
  case file_types::TY_YAMLOptRecord:
  case file_types::TY_BitstreamOptRecord:
  case file_types::TY_SwiftModuleInterfaceFile:
  case file_types::TY_PrivateSwiftModuleInterfaceFile:
  case file_types::TY_PackageSwiftModuleInterfaceFile:
  case file_types::TY_JSONDependencies:
  case file_types::TY_JSONArguments:
  case file_types::TY_IndexUnitOutputPath:
  case file_types::TY_SwiftABIDescriptor:
  case file_types::TY_SwiftAPIDescriptor:
  case file_types::TY_ConstValues:
  case file_types::TY_SwiftFixIt:
  case file_types::TY_ModuleSemanticInfo:
  case file_types::TY_CachedDiagnostics:
  case file_types::TY_SymbolGraphFile:
    return false;
  case file_types::TY_INVALID:
    llvm_unreachable("Invalid type ID.");
  }

  // Work around MSVC warning: not all control paths return a value
  llvm_unreachable("All switch cases are covered");
}

bool file_types::isPartOfSwiftCompilation(ID Id) {
  switch (Id) {
  case file_types::TY_Swift:
  case file_types::TY_SIL:
  case file_types::TY_LoweredSIL:
  case file_types::TY_RawSIL:
  case file_types::TY_SIB:
  case file_types::TY_RawSIB:
    return true;
  case file_types::TY_Assembly:
  case file_types::TY_RawLLVM_IR:
  case file_types::TY_LLVM_IR:
  case file_types::TY_LLVM_BC:
  case file_types::TY_Object:
  case file_types::TY_Dependencies:
  case file_types::TY_ClangHeader:
  case file_types::TY_AutolinkFile:
  case file_types::TY_PCH:
  case file_types::TY_ImportedModules:
  case file_types::TY_TBD:
  case file_types::TY_Image:
  case file_types::TY_dSYM:
  case file_types::TY_SwiftModuleFile:
  case file_types::TY_SwiftModuleDocFile:
  case file_types::TY_SwiftModuleInterfaceFile:
  case file_types::TY_PrivateSwiftModuleInterfaceFile:
  case file_types::TY_PackageSwiftModuleInterfaceFile:
  case file_types::TY_SwiftSourceInfoFile:
  case file_types::TY_SwiftCrossImportDir:
  case file_types::TY_SwiftOverlayFile:
  case file_types::TY_SwiftModuleSummaryFile:
  case file_types::TY_SerializedDiagnostics:
  case file_types::TY_ClangModuleFile:
  case file_types::TY_SwiftDeps:
  case file_types::TY_ExternalSwiftDeps:
  case file_types::TY_Nothing:
  case file_types::TY_ASTDump:
  case file_types::TY_Remapping:
  case file_types::TY_IndexData:
  case file_types::TY_ModuleTrace:
  case file_types::TY_FineModuleTrace:
  case file_types::TY_YAMLOptRecord:
  case file_types::TY_BitstreamOptRecord:
  case file_types::TY_JSONDependencies:
  case file_types::TY_JSONArguments:
  case file_types::TY_IndexUnitOutputPath:
  case file_types::TY_SwiftABIDescriptor:
  case file_types::TY_SwiftAPIDescriptor:
  case file_types::TY_ConstValues:
  case file_types::TY_SwiftFixIt:
  case file_types::TY_ModuleSemanticInfo:
  case file_types::TY_CachedDiagnostics:
  case file_types::TY_SymbolGraphFile:
    return false;
  case file_types::TY_INVALID:
    llvm_unreachable("Invalid type ID.");
  }

  // Work around MSVC warning: not all control paths return a value
  llvm_unreachable("All switch cases are covered");
}

bool file_types::isProducedFromDiagnostics(ID Id) {
  switch (Id) {
  case file_types::TY_SerializedDiagnostics:
  case file_types::TY_SwiftFixIt:
  case file_types::TY_CachedDiagnostics:
    return true;
  case file_types::TY_Swift:
  case file_types::TY_SIL:
  case file_types::TY_LoweredSIL:
  case file_types::TY_RawSIL:
  case file_types::TY_SIB:
  case file_types::TY_RawSIB:
  case file_types::TY_Assembly:
  case file_types::TY_RawLLVM_IR:
  case file_types::TY_LLVM_IR:
  case file_types::TY_LLVM_BC:
  case file_types::TY_Object:
  case file_types::TY_Dependencies:
  case file_types::TY_ClangHeader:
  case file_types::TY_AutolinkFile:
  case file_types::TY_PCH:
  case file_types::TY_ImportedModules:
  case file_types::TY_TBD:
  case file_types::TY_Image:
  case file_types::TY_dSYM:
  case file_types::TY_SwiftModuleFile:
  case file_types::TY_SwiftModuleDocFile:
  case file_types::TY_SwiftModuleInterfaceFile:
  case file_types::TY_PrivateSwiftModuleInterfaceFile:
  case file_types::TY_PackageSwiftModuleInterfaceFile:
  case file_types::TY_SwiftSourceInfoFile:
  case file_types::TY_SwiftCrossImportDir:
  case file_types::TY_SwiftOverlayFile:
  case file_types::TY_SwiftModuleSummaryFile:
  case file_types::TY_ClangModuleFile:
  case file_types::TY_SwiftDeps:
  case file_types::TY_ExternalSwiftDeps:
  case file_types::TY_Nothing:
  case file_types::TY_ASTDump:
  case file_types::TY_Remapping:
  case file_types::TY_IndexData:
  case file_types::TY_ModuleTrace:
  case file_types::TY_FineModuleTrace:
  case file_types::TY_YAMLOptRecord:
  case file_types::TY_BitstreamOptRecord:
  case file_types::TY_JSONDependencies:
  case file_types::TY_JSONArguments:
  case file_types::TY_IndexUnitOutputPath:
  case file_types::TY_SwiftABIDescriptor:
  case file_types::TY_SwiftAPIDescriptor:
  case file_types::TY_ConstValues:
  case file_types::TY_ModuleSemanticInfo:
  case file_types::TY_SymbolGraphFile:
    return false;
  case file_types::TY_INVALID:
    llvm_unreachable("Invalid type ID.");
  }

  // Work around MSVC warning: not all control paths return a value
  llvm_unreachable("All switch cases are covered");
}
