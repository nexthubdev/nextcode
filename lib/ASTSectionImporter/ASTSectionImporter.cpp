//===--- ASTSectionImporter.cpp - Import AST Section Modules --------------===//
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
// This file implements support for loading modules serialized into a
// Mach-O AST section into Swift.
//
//===----------------------------------------------------------------------===//

#include "language/ASTSectionImporter/ASTSectionImporter.h"
#include "../Serialization/ModuleFormat.h"
#include "language/AST/ASTContext.h"
#include "language/Basic/Assertions.h"
#include "language/Serialization/SerializedModuleLoader.h"
#include "language/Serialization/Validation.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace language;

std::string ASTSectionParseError::toString() const {
  std::string S;
  llvm::raw_string_ostream SS(S);
  SS << serialization::StatusToString(Error);
  if (!ErrorMessage.empty())
    SS << ": " << ErrorMessage;
  return SS.str();
}

void ASTSectionParseError::log(raw_ostream &OS) const { OS << toString(); }

std::error_code ASTSectionParseError::convertToErrorCode() const {
  llvm_unreachable("Function not implemented.");
}

char ASTSectionParseError::ID;

llvm::Expected<SmallVector<std::string, 4>>
swift::parseASTSection(MemoryBufferSerializedModuleLoader &Loader,
                       StringRef buf,
                       const llvm::Triple &filter) {
  if (!serialization::isSerializedAST(buf))
    return llvm::make_error<ASTSectionParseError>(
        serialization::Status::Malformed);

  SmallVector<std::string, 4> foundModules;
  bool haveFilter = filter.getOS() != llvm::Triple::UnknownOS &&
                   filter.getArch() != llvm::Triple::UnknownArch;
  // An AST section consists of one or more AST modules, optionally with
  // headers. Iterate over all AST modules.
  while (!buf.empty()) {
    auto info = serialization::validateSerializedAST(
        buf, Loader.isRequiredOSSAModules(),
        /*requiredSDK*/StringRef());

    assert(info.name.size() < (2 << 10) && "name failed sanity check");

    std::string error;
    llvm::raw_string_ostream errs(error);
    if (info.status == serialization::Status::Valid) {
      assert(info.bytes != 0);
      bool selected = true;
      if (haveFilter) {
        llvm::Triple moduleTriple(info.targetTriple);
        selected = serialization::areCompatible(moduleTriple, filter);
      }
      if (!info.name.empty() && selected) {
        StringRef moduleData = buf.substr(0, info.bytes);
        std::unique_ptr<llvm::MemoryBuffer> bitstream(
            llvm::MemoryBuffer::getMemBuffer(moduleData, info.name, false));

        // Register the memory buffer.
        Loader.registerMemoryBuffer(info.name, std::move(bitstream),
                                    info.userModuleVersion);
        foundModules.push_back(info.name.str());
      }
    } else {
      errs << "Unable to load module";
      if (!info.name.empty())
        errs << " '" << info.name << '\'';
      errs << ".";
    }

    if (info.bytes == 0)
      return llvm::make_error<ASTSectionParseError>(info.status, errs.str());

    if (info.bytes > buf.size()) {
      errs << "AST section too small.";
      return llvm::make_error<ASTSectionParseError>(
          serialization::Status::Malformed, errs.str());
    }

    buf = buf.substr(
      llvm::alignTo(info.bytes, swift::serialization::SWIFTMODULE_ALIGNMENT));
  }

  return foundModules;
}

bool swift::parseASTSection(MemoryBufferSerializedModuleLoader &Loader,
                            StringRef buf,
                            const llvm::Triple &filter,
                            SmallVectorImpl<std::string> &foundModules) {
  auto Result = parseASTSection(Loader, buf, filter);
  if (auto E = Result.takeError()) {
    llvm::dbgs() << toString(std::move(E));
    return false;
  }
  for (auto m : *Result)
    foundModules.push_back(m);
  return true;
}
