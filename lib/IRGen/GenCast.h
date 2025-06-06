//===--- GenCast.h - Swift IR generation for dynamic casts ------*- C++ -*-===//
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
//  This file provides the private interface to the dynamic cast code.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_IRGEN_GENCAST_H
#define SWIFT_IRGEN_GENCAST_H

#include "language/AST/Types.h"

namespace llvm {
  class Value;
  class BasicBlock;
}

namespace language {
  class SILType;
  class ProtocolDecl;
  enum class CastConsumptionKind : unsigned char;
  enum class CastingIsolatedConformances: uint8_t;

namespace irgen {
  class Address;
  class IRGenFunction;
  class Explosion;

  /// Discriminator for checked cast modes.
  enum class CheckedCastMode : uint8_t {
    Unconditional,
    Conditional,
  };

  llvm::Value *emitCheckedCast(IRGenFunction &IGF,
                               Address src,
                               CanType fromType,
                               Address dest,
                               CanType toType,
                               CastConsumptionKind consumptionKind,
                               CheckedCastMode mode,
                              CastingIsolatedConformances isolatedConformances);

  void emitScalarCheckedCast(IRGenFunction &IGF, Explosion &value,
                             SILType sourceLoweredType,
                             CanType sourceFormalType,
                             SILType targetLoweredType,
                             CanType targetFormalType,
                             CheckedCastMode mode,
                             CastingIsolatedConformances isolatedConformances,
                             Explosion &out);

  llvm::Value *emitFastClassCastIfPossible(
      IRGenFunction &IGF, llvm::Value *instance, CanType sourceFormalType,
      CanType targetFormalType, bool sourceWrappedInOptional,
      llvm::BasicBlock *&nilCheckBB, llvm::BasicBlock *&nilMergeBB);

  /// Convert a class object to the given destination type,
  /// using a runtime-checked cast.
  llvm::Value *emitClassDowncast(IRGenFunction &IGF,
                                 llvm::Value *from,
                                 CanType toType,
                                 CheckedCastMode mode);

  /// A result of a cast generation function.
  struct FailableCastResult {
    /// An i1 value that's set to True if the cast succeeded.
    llvm::Value *succeeded;
    /// On success, this value stores the result of the cast operation.
    llvm::Value *casted;
  };

  /// Convert the given value to the exact destination type.
  FailableCastResult emitClassIdenticalCast(IRGenFunction &IGF,
                                            llvm::Value *from,
                                            SILType fromType,
                                            SILType toType);

  /// Emit a checked cast of a metatype.
  void emitMetatypeDowncast(IRGenFunction &IGF,
                            llvm::Value *metatype,
                            CanMetatypeType toMetatype,
                            CheckedCastMode mode,
                            Explosion &ex);

  /// Emit a checked cast to a class-constrained protocol or protocol
  /// composition.
  ///
  /// If a metatype kind is provided, the cast is done as a metatype cast. If
  /// not, the cast is done as a class instance cast.
  void emitScalarExistentialDowncast(
      IRGenFunction &IGF, llvm::Value *orig, SILType srcType, SILType destType,
      CheckedCastMode mode, std::optional<MetatypeRepresentation> metatypeKind,
      Explosion &ex);

  /// Emit a checked cast from a metatype to AnyObject.
  llvm::Value *emitMetatypeToAnyObjectDowncast(IRGenFunction &IGF,
                                            llvm::Value *metatypeValue,
                                            CanAnyMetatypeType type,
                                            CheckedCastMode mode);

  /// Emit a Protocol* value referencing an ObjC protocol.
  llvm::Value *emitReferenceToObjCProtocol(IRGenFunction &IGF,
                                           ProtocolDecl *proto);
} // end namespace irgen
} // end namespace language

#endif
