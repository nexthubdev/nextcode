//===--- CodeCompletionConsumer.h -----------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2022 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_IDE_CODECOMPLETIONCONSUMER
#define SWIFT_IDE_CODECOMPLETIONCONSUMER

#include "language/IDE/CodeCompletionContext.h"
#include "language/Parse/IDEInspectionCallbacks.h"

namespace language {
namespace ide {

struct RequestedCachedModule;

/// An abstract base class for consumers of code completion results.
class CodeCompletionConsumer {
public:
  virtual ~CodeCompletionConsumer() {}
  /// Clients should override this method to receive \p Results.
  virtual void handleResults(CodeCompletionContext &context) = 0;
};

} // end namespace ide
} // end namespace language

#endif // SWIFT_IDE_CODECOMPLETIONCONSUMER
