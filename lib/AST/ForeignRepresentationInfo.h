//===--- ForeignRepresentationInfo.h - Used in bridging queries -*- C++ -*-===//
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

#ifndef SWIFT_FOREIGNREPRESENTATIONINFO_H
#define SWIFT_FOREIGNREPRESENTATIONINFO_H

#include "language/AST/Type.h"
#include "language/Basic/LLVM.h"
#include "llvm/ADT/PointerEmbeddedInt.h"
#include "llvm/ADT/PointerIntPair.h"

namespace language {

class ProtocolConformance;

class ForeignRepresentationInfo {
  using PayloadTy =
    llvm::PointerEmbeddedInt<uintptr_t, sizeof(uintptr_t) * CHAR_BIT - 3>;

  /// The low three bits store a ForeignRepresentableKind.
  ///
  /// When the ForeignRepresentableKind == None, the upper bits are
  /// the generation count at which this negative result was last checked.
  /// When it's Bridged, it's the conformance that describes the bridging.
  /// When it's Trivial, it's simply a flag stating whether Optional is
  /// supported.
  llvm::PointerIntPair<PayloadTy, 3, ForeignRepresentableKind> Storage;

public:
  /// Retrieve a cache entry for a non-foreign-representable type.
  static ForeignRepresentationInfo forNone(unsigned generation = 0) {
    ForeignRepresentationInfo result;
    result.Storage = { generation, ForeignRepresentableKind::None };
    return result;
  }

  // Retrieve a cache entry for a trivially representable type.
  static ForeignRepresentationInfo forTrivial() {
    ForeignRepresentationInfo result;
    result.Storage = { 0, ForeignRepresentableKind::Trivial };
    return result;
  }

  // Retrieve a cache entry for a trivially representable type that can also
  // be optional.
  static ForeignRepresentationInfo forTrivialWithOptional() {
    ForeignRepresentationInfo result;
    result.Storage = { 1, ForeignRepresentableKind::Trivial };
    return result;
  }

  // Retrieve a cache entry for a bridged representable type.
  static ForeignRepresentationInfo
  forBridged(ProtocolConformance *conformance) {
    using PayloadTraits = llvm::PointerLikeTypeTraits<PayloadTy>;
    ForeignRepresentationInfo result;
    result.Storage = {PayloadTraits::getFromVoidPointer(conformance),
                      ForeignRepresentableKind::Bridged};
    assert(result.getConformance() == conformance && "insufficiently aligned");
    return result;
  }

  // Retrieve a cache entry for a trivially representable type that can also
  // be optional.
  static ForeignRepresentationInfo forBridgedError() {
    ForeignRepresentationInfo result;
    result.Storage = { 0, ForeignRepresentableKind::BridgedError };
    return result;
  }

  /// Retrieve the foreign representable kind.
  ForeignRepresentableKind getKind() const {
    return Storage.getInt();
  }

  /// Retrieve the generation for a non-representable type.
  unsigned getGeneration() const {
    assert(getKind() == ForeignRepresentableKind::None);
    return Storage.getPointer();
  }

  /// Retrieve the protocol conformance that makes it representable.
  ProtocolConformance *getConformance() const {
    switch (getKind()) {
    case ForeignRepresentableKind::None:
      llvm_unreachable("this type is not representable");

    case ForeignRepresentableKind::Trivial:
    case ForeignRepresentableKind::BridgedError:
      return nullptr;

    case ForeignRepresentableKind::Bridged: {
      using PayloadTraits = llvm::PointerLikeTypeTraits<PayloadTy>;
      auto payload = PayloadTraits::getAsVoidPointer(Storage.getPointer());
      return static_cast<ProtocolConformance *>(payload);
    }

    case ForeignRepresentableKind::Object:
    case ForeignRepresentableKind::StaticBridged:
      llvm_unreachable("unexpected kind in ForeignRepresentableCacheEntry");
    }

    llvm_unreachable("Unhandled ForeignRepresentableKind in switch.");
  }

  /// Returns true if the optional version of this type is also representable.
  bool isRepresentableAsOptional() const;
};

} // end namespace language

#endif // SWIFT_FOREIGNREPRESENTATIONINFO_H
