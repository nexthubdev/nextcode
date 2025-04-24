//===--- ConcreteDeclRef.cpp - Reference to a concrete decl ---------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file implements the ConcreteDeclRef class, which provides a reference to
// a declaration that is potentially specialized.
//
//===----------------------------------------------------------------------===//

#include "language/AST/ASTContext.h"
#include "language/AST/ConcreteDeclRef.h"
#include "language/AST/Decl.h"
#include "language/AST/GenericSignature.h"
#include "language/AST/ProtocolConformance.h"
#include "language/AST/SubstitutionMap.h"
#include "language/AST/Types.h"
#include "language/Basic/Assertions.h"
#include "llvm/Support/raw_ostream.h"
using namespace language;

ConcreteDeclRef ConcreteDeclRef::getOverriddenDecl() const {
  auto *derivedDecl = getDecl();
  auto *baseDecl = derivedDecl->getOverriddenDecl();

  auto baseSig = baseDecl->getInnermostDeclContext()
      ->getGenericSignatureOfContext();
  auto derivedSig = derivedDecl->getInnermostDeclContext()
      ->getGenericSignatureOfContext();

  SubstitutionMap subs;
  if (baseSig) {
    subs = SubstitutionMap::getOverrideSubstitutions(baseDecl, derivedDecl);
    if (derivedSig)
      subs = subs.subst(getSubstitutions());
  }
  return ConcreteDeclRef(baseDecl, subs);
}

ConcreteDeclRef ConcreteDeclRef::getOverriddenDecl(ValueDecl *baseDecl) const {
  auto *derivedDecl = getDecl();
  if (baseDecl == derivedDecl) return *this;

#ifndef NDEBUG
  {
    auto cur = derivedDecl;
    for (; cur && cur != baseDecl; cur = cur->getOverriddenDecl()) {}
    assert(cur && "decl is not an indirect override of baseDecl");
  }
#endif

  if (!baseDecl->getInnermostDeclContext()->isGenericContext()) {
    return ConcreteDeclRef(baseDecl);
  }

  auto subs = SubstitutionMap::getOverrideSubstitutions(baseDecl, derivedDecl);
  if (auto derivedSubs = getSubstitutions()) {
    subs = subs.subst(derivedSubs);
  }
  return ConcreteDeclRef(baseDecl, subs);
}

void ConcreteDeclRef::dump(raw_ostream &os) const {
  if (!getDecl()) {
    os << "**NULL**";
    return;
  }

  getDecl()->dumpRef(os);

  // If specialized, dump the substitutions.
  if (isSpecialized()) {
    os << " [with ";
    getSubstitutions().dump(os, SubstitutionMap::DumpStyle::Minimal);
    os << ']';
  }
}

void ConcreteDeclRef::dump() const {
  dump(llvm::errs());
}
