//===--- OutputFileMap.h - Map of inputs to multiple outputs ----*- C++ -*-===//
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

#ifndef SWIFT_BASIC_OUTPUTFILEMAP_H
#define SWIFT_BASIC_OUTPUTFILEMAP_H

#include "language/Basic/FileTypes.h"
#include "language/Basic/LLVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"

#include <memory>
#include <string>

namespace language {

using TypeToPathMap = llvm::DenseMap<file_types::ID, std::string>;

/// A two-tiered map used to specify paths for multiple output files associated
/// with each input file in a compilation job.
///
/// The structure is a map from input paths to sub-maps, each of which maps
/// file types to output paths.
class OutputFileMap {
private:
  llvm::StringMap<TypeToPathMap> InputToOutputsMap;

public:
  OutputFileMap() {}

  ~OutputFileMap() = default;

  /// Loads an OutputFileMap from the given \p Path into the receiver, if
  /// possible.
  static llvm::Expected<OutputFileMap>
  loadFromPath(StringRef Path, StringRef workingDirector);

  static llvm::Expected<OutputFileMap>
  loadFromBuffer(StringRef Data, StringRef workingDirectory);

  /// Loads an OutputFileMap from the given \p Buffer, taking ownership
  /// of the buffer in the process.
  ///
  /// When non-empty, \p workingDirectory is used to resolve relative paths in
  /// the output file map.
  static llvm::Expected<OutputFileMap>
  loadFromBuffer(std::unique_ptr<llvm::MemoryBuffer> Buffer,
                 StringRef workingDirectory);

  /// Get the map of outputs for the given \p Input, if present in the
  /// OutputFileMap. (If not present, returns nullptr.)
  const TypeToPathMap *getOutputMapForInput(StringRef Input) const;

  /// Get a map of outputs for the given \p Input, creating it in
  /// the OutputFileMap if not already present.
  TypeToPathMap &getOrCreateOutputMapForInput(StringRef Input);

  /// Get the map of outputs for a single compile product.
  const TypeToPathMap *getOutputMapForSingleOutput() const;

  /// Get or create the map of outputs for a single compile product.
  TypeToPathMap &getOrCreateOutputMapForSingleOutput();

  /// Dump the OutputFileMap to the given \p os.
  void dump(llvm::raw_ostream &os, bool Sort = false) const;

  /// Write the OutputFileMap for the \p inputs so it can be parsed.
  ///
  /// It is not an error if the map does not contain an entry for a particular
  /// input. Instead, an empty sub-map will be written into the output.
  void write(llvm::raw_ostream &os, ArrayRef<StringRef> inputs) const;

private:
  /// Parses the given \p Buffer and returns either an OutputFileMap or
  /// error, taking ownership of \p Buffer in the process.
  static llvm::Expected<OutputFileMap>
  parse(std::unique_ptr<llvm::MemoryBuffer> Buffer, StringRef workingDirectory);
};

} // end namespace language

#endif
