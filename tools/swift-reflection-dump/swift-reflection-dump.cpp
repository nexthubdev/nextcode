//===--- swift-reflection-dump.cpp - Reflection testing application -------===//
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
// This is a host-side tool to dump remote reflection sections in swift
// binaries.
//===----------------------------------------------------------------------===//

#include "language/ABI/MetadataValues.h"
#include "language/Basic/LLVMInitialize.h"
#include "language/Demangling/Demangle.h"
#include "language/RemoteInspection/ReflectionContext.h"
#include "language/RemoteInspection/TypeRef.h"
#include "language/RemoteInspection/TypeRefBuilder.h"
#include "language/StaticMirror/ObjectFileContext.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ELF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/MachOUniversal.h"
#include "llvm/Object/RelocationResolver.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#endif

#include <algorithm>
#include <csignal>
#include <iostream>

using llvm::ArrayRef;
using llvm::dyn_cast;
using llvm::StringRef;
using namespace llvm::object;

using namespace language;
using namespace language::reflection;
using namespace language::static_mirror;
using namespace language::remote;
using namespace Demangle;

enum class ActionType { DumpReflectionSections, DumpTypeLowering };

namespace options {
static llvm::cl::opt<ActionType> Action(
    llvm::cl::desc("Mode:"),
    llvm::cl::values(
        clEnumValN(ActionType::DumpReflectionSections,
                   "dump-reflection-sections",
                   "Dump the field reflection section"),
        clEnumValN(
            ActionType::DumpTypeLowering, "dump-type-lowering",
            "Dump the field layout for typeref strings read from stdin")),
    llvm::cl::init(ActionType::DumpReflectionSections));

static llvm::cl::list<std::string>
BinaryFilename(llvm::cl::Positional,
                   llvm::cl::desc("Filenames of the binary files"),
                   llvm::cl::OneOrMore);

static llvm::cl::opt<std::string>
    Architecture("arch",
                 llvm::cl::desc("Architecture to inspect in the binary"),
                 llvm::cl::Required);

#if SWIFT_OBJC_INTEROP
static llvm::cl::opt<bool> DisableObjCInterop(
    "no-objc-interop",
    llvm::cl::desc("Disable Objective-C interoperability support"));
#endif
} // end namespace options

static int doDumpReflectionSections(ArrayRef<std::string> BinaryFilenames,
                                    StringRef Arch, ActionType Action,
                                    std::ostream &stream) {
  // Note: binaryOrError and objectOrError own the memory for our ObjectFile;
  // once they go out of scope, we can no longer do anything.
  std::vector<OwningBinary<Binary>> BinaryOwners;
  std::vector<std::unique_ptr<ObjectFile>> ObjectOwners;
  std::vector<const ObjectFile *> ObjectFiles;

  for (const std::string &BinaryFilename : BinaryFilenames) {
    auto BinaryOwner = unwrap(createBinary(BinaryFilename));
    Binary *BinaryFile = BinaryOwner.getBinary();

    // The object file we are doing lookups in -- either the binary itself, or
    // a particular slice of a universal binary.
    std::unique_ptr<ObjectFile> ObjectOwner;
    const ObjectFile *O = dyn_cast<ObjectFile>(BinaryFile);
    if (!O) {
      auto Universal = cast<MachOUniversalBinary>(BinaryFile);
      ObjectOwner = unwrap(Universal->getMachOObjectForArch(Arch));
      O = ObjectOwner.get();
    }

    // Retain the objects that own section memory
    BinaryOwners.push_back(std::move(BinaryOwner));
    ObjectOwners.push_back(std::move(ObjectOwner));
    ObjectFiles.push_back(O);
  }

#if SWIFT_OBJC_INTEROP
  bool ObjCInterop = !options::DisableObjCInterop;
#else
  bool ObjCInterop = false;
#endif
  auto context = makeReflectionContextForObjectFiles(ObjectFiles, ObjCInterop);
  auto &builder = context->Builder;

  switch (Action) {
  case ActionType::DumpReflectionSections:
    // Dump everything
    switch (context->PointerSize) {
    case 4:
#if SWIFT_OBJC_INTEROP
      if (!options::DisableObjCInterop)
        builder.dumpAllSections<WithObjCInterop, 4>(stream);
      else
        builder.dumpAllSections<NoObjCInterop, 4>(stream);
#else
      builder.dumpAllSections<NoObjCInterop, 4>(stream);
#endif
      break;
    case 8:
#if SWIFT_OBJC_INTEROP
      if (!options::DisableObjCInterop)
        builder.dumpAllSections<WithObjCInterop, 8>(stream);
      else
        builder.dumpAllSections<NoObjCInterop, 8>(stream);
#else
      builder.dumpAllSections<NoObjCInterop, 8>(stream);
#endif
      break;
    default:
      fputs("unsupported word size in object file\n", stderr);
      abort();
    }
    break;
  case ActionType::DumpTypeLowering: {
    for (std::string Line; std::getline(std::cin, Line);) {
      if (Line.empty())
        continue;

      if (StringRef(Line).starts_with("//"))
        continue;

      Demangle::Demangler Dem;
      auto Demangled = Dem.demangleType(Line);
      auto Result = swift::Demangle::decodeMangledType(builder, Demangled);
      if (Result.isError()) {
        auto *error = Result.getError();
        char *str = error->copyErrorString();
        stream << "Invalid typeref:" << Line << " - " << str << "\n";
        error->freeErrorString(str);
        continue;
      }
      auto TypeRef = Result.getType();

      TypeRef->dump(stream);
      auto *TypeInfo = builder.getTypeConverter().getTypeInfo(TypeRef, nullptr);
      if (TypeInfo == nullptr) {
        stream << "Invalid lowering\n";
        continue;
      }
      TypeInfo->dump(stream);
    }
    break;
  }
  }

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  PROGRAM_START(argc, argv);
  llvm::cl::ParseCommandLineOptions(argc, argv, "Swift Reflection Dump\n");
  return doDumpReflectionSections(options::BinaryFilename,
                                  options::Architecture, options::Action,
                                  std::cout);
}
