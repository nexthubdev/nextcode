//===--------------------------------------------------------------------===////
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2023 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_REFACTORING_REFACTORINGACTIONS_H
#define SWIFT_REFACTORING_REFACTORINGACTIONS_H

#include "language/AST/ASTContext.h"
#include "language/AST/SourceFile.h"
#include "language/Basic/SourceManager.h"
#include "language/IDE/IDERequests.h"
#include "language/Parse/Lexer.h"
#include "language/Refactoring/Refactoring.h"

namespace language {
namespace refactoring {

using namespace language;
using namespace language::ide;

class RefactoringAction {
protected:
  ModuleDecl *MD;
  SourceFile *TheFile;
  SourceEditConsumer &EditConsumer;
  ASTContext &Ctx;
  SourceManager &SM;
  DiagnosticEngine DiagEngine;
  SourceLoc StartLoc;
  StringRef PreferredName;

public:
  RefactoringAction(ModuleDecl *MD, RefactoringOptions &Opts,
                    SourceEditConsumer &EditConsumer,
                    DiagnosticConsumer &DiagConsumer);
  virtual ~RefactoringAction() = default;
  virtual bool performChange() = 0;
};

/// Different from RangeBasedRefactoringAction, TokenBasedRefactoringAction
/// takes the input of a given token, e.g., a name or an "if" key word.
/// Contextual refactoring kinds can suggest applicable refactorings on that
/// token, e.g. rename or reverse if statement.
class TokenBasedRefactoringAction : public RefactoringAction {
protected:
  ResolvedCursorInfoPtr CursorInfo;

public:
  TokenBasedRefactoringAction(ModuleDecl *MD, RefactoringOptions &Opts,
                              SourceEditConsumer &EditConsumer,
                              DiagnosticConsumer &DiagConsumer);
};

#define CURSOR_REFACTORING(KIND, NAME, ID)                                     \
  class RefactoringAction##KIND : public TokenBasedRefactoringAction {         \
  public:                                                                      \
    RefactoringAction##KIND(ModuleDecl *MD, RefactoringOptions &Opts,          \
                            SourceEditConsumer &EditConsumer,                  \
                            DiagnosticConsumer &DiagConsumer)                  \
        : TokenBasedRefactoringAction(MD, Opts, EditConsumer, DiagConsumer) {} \
    bool performChange() override;                                             \
    static bool isApplicable(ResolvedCursorInfoPtr Info,                       \
                             DiagnosticEngine &Diag);                          \
    bool isApplicable() {                                                      \
      return RefactoringAction##KIND::isApplicable(CursorInfo, DiagEngine);    \
    }                                                                          \
  };
#include "language/Refactoring/RefactoringKinds.def"

class RangeBasedRefactoringAction : public RefactoringAction {
protected:
  ResolvedRangeInfo RangeInfo;

public:
  RangeBasedRefactoringAction(ModuleDecl *MD, RefactoringOptions &Opts,
                              SourceEditConsumer &EditConsumer,
                              DiagnosticConsumer &DiagConsumer);
};

#define RANGE_REFACTORING(KIND, NAME, ID)                                      \
  class RefactoringAction##KIND : public RangeBasedRefactoringAction {         \
  public:                                                                      \
    RefactoringAction##KIND(ModuleDecl *MD, RefactoringOptions &Opts,          \
                            SourceEditConsumer &EditConsumer,                  \
                            DiagnosticConsumer &DiagConsumer)                  \
        : RangeBasedRefactoringAction(MD, Opts, EditConsumer, DiagConsumer) {} \
    bool performChange() override;                                             \
    static bool isApplicable(const ResolvedRangeInfo &Info,                    \
                             DiagnosticEngine &Diag);                          \
    bool isApplicable() {                                                      \
      return RefactoringAction##KIND::isApplicable(RangeInfo, DiagEngine);     \
    }                                                                          \
  };
#include "language/Refactoring/RefactoringKinds.def"

} // end namespace refactoring
} // end namespace language

#endif
