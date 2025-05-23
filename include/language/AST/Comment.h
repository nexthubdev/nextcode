//===--- Comment.h - Swift-specific comment parsing -------------*- C++ -*-===//
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

#ifndef SWIFT_AST_COMMENT_H
#define SWIFT_AST_COMMENT_H

#include "language/Markup/Markup.h"
#include <optional>

namespace language {
class Decl;
class TypeDecl;
struct RawComment;

class DocComment {
  const Decl *D;
  swift::markup::Document *Doc = nullptr;
  swift::markup::CommentParts Parts;

  DocComment(const Decl *D, swift::markup::Document *Doc,
             swift::markup::CommentParts Parts)
      : D(D), Doc(Doc), Parts(Parts) {}

public:
  static DocComment *create(const Decl *D, swift::markup::MarkupContext &MC,
                            RawComment RC);

  void addInheritanceNote(swift::markup::MarkupContext &MC, TypeDecl *base);

  const Decl *getDecl() const { return D; }
  void setDecl(const Decl *D) { this->D = D; }

  const swift::markup::Document *getDocument() const { return Doc; }

  swift::markup::CommentParts getParts() const {
    return Parts;
  }

  ArrayRef<StringRef> getTags() const {
    return llvm::ArrayRef(Parts.Tags.begin(), Parts.Tags.end());
  }

  std::optional<const swift::markup::Paragraph *> getBrief() const {
    return Parts.Brief;
  }

  std::optional<const swift::markup::ReturnsField *> getReturnsField() const {
    return Parts.ReturnsField;
  }

  std::optional<const swift::markup::ThrowsField *> getThrowsField() const {
    return Parts.ThrowsField;
  }

  ArrayRef<const swift::markup::ParamField *> getParamFields() const {
    return Parts.ParamFields;
  }

  ArrayRef<const swift::markup::MarkupASTNode *> getBodyNodes() const {
    return Parts.BodyNodes;
  }

  std::optional<const markup::LocalizationKeyField *>
  getLocalizationKeyField() const {
    return Parts.LocalizationKeyField;
  }

  bool isEmpty() const {
    return Parts.isEmpty();
  }

  // Only allow allocation using the allocator in MarkupContext or by
  // placement new.
  void *operator new(size_t Bytes, swift::markup::MarkupContext &MC,
                     unsigned Alignment = alignof(DocComment));
  void *operator new(size_t Bytes, void *Mem) {
    assert(Mem);
    return Mem;
  }

  // Make vanilla new/delete illegal.
  void *operator new(size_t Bytes) = delete;
  void operator delete(void *Data) = delete;
};

/// Get a parsed documentation comment for the declaration, if there is one.
DocComment *getSingleDocComment(swift::markup::MarkupContext &Context,
                                const Decl *D);

/// Extract comments parts from the given Markup node.
swift::markup::CommentParts
extractCommentParts(swift::markup::MarkupContext &MC,
                    swift::markup::MarkupASTNode *Node);

/// Extract brief comment from \p RC, and print it to \p OS .
void printBriefComment(RawComment RC, llvm::raw_ostream &OS);

/// Describes the intended serialization target for a doc comment.
enum class DocCommentSerializationTarget : uint8_t {
  /// The doc comment should not be serialized.
  None = 0,

  /// The doc comment should only be serialized in the 'swiftsourceinfo'.
  SourceInfoOnly,

  /// The doc comment should be serialized in both the 'swiftdoc' and
  /// 'swiftsourceinfo'.
  SwiftDocAndSourceInfo,
};

/// Retrieve the expected serialization target for a documentation comment
/// attached to the given decl.
DocCommentSerializationTarget
getDocCommentSerializationTargetFor(const Decl *D);

} // namespace language

#endif // LLVM_SWIFT_AST_COMMENT_H
