//===--- CodeCompletionCache.h - Routines for code completion caching -----===//
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

#ifndef SWIFT_IDE_CODE_COMPLETIONCACHE_H
#define SWIFT_IDE_CODE_COMPLETIONCACHE_H

#include "language/Basic/ThreadSafeRefCounted.h"
#include "language/IDE/CodeCompletion.h"
#include "language/IDE/CodeCompletionResult.h"
#include "language/IDE/CodeCompletionString.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Support/Chrono.h"
#include <system_error>

namespace language {
namespace ide {

struct CodeCompletionCacheImpl;
class OnDiskCodeCompletionCache;

/// In-memory per-module code completion result cache.
///
/// These results persist between multiple code completion requests and can be
/// used with different ASTContexts.
class CodeCompletionCache {
  std::unique_ptr<CodeCompletionCacheImpl> Impl;
  OnDiskCodeCompletionCache *nextCache;

public:
  /// Cache key.
  struct Key {
    std::string ModuleFilename;
    std::string ModuleName;
    std::vector<std::string> AccessPath;
    bool ResultsHaveLeadingDot;
    bool ForTestableLookup;
    bool ForPrivateImportLookup;
    /// Must be sorted alphabetically for stable identity.
    llvm::SmallVector<std::string, 2> SpiGroups;
    bool AddInitsInToplevel;
    bool AddCallWithNoDefaultArgs;
    bool Annotated;

    friend bool operator==(const Key &LHS, const Key &RHS) {
      return LHS.ModuleFilename == RHS.ModuleFilename &&
             LHS.ModuleName == RHS.ModuleName &&
             LHS.AccessPath == RHS.AccessPath &&
             LHS.ResultsHaveLeadingDot == RHS.ResultsHaveLeadingDot &&
             LHS.ForTestableLookup == RHS.ForTestableLookup &&
             LHS.ForPrivateImportLookup == RHS.ForPrivateImportLookup &&
             LHS.SpiGroups == RHS.SpiGroups &&
             LHS.AddInitsInToplevel == RHS.AddInitsInToplevel &&
             LHS.AddCallWithNoDefaultArgs == RHS.AddCallWithNoDefaultArgs &&
             LHS.Annotated == RHS.Annotated;
    }
  };

  struct Value : public llvm::ThreadSafeRefCountedBase<Value> {
    /// The allocator used to allocate the results stored in this cache.
    std::shared_ptr<llvm::BumpPtrAllocator> Allocator;

    llvm::sys::TimePoint<> ModuleModificationTime;

    std::vector<const ContextFreeCodeCompletionResult *> Results;

    /// The arena that contains the \c USRBasedTypes of the
    /// \c ContextFreeCodeCompletionResult in this cache value.
    USRBasedTypeArena USRTypeArena;

    Value() : Allocator(std::make_shared<llvm::BumpPtrAllocator>()) {}
  };
  using ValueRefCntPtr = llvm::IntrusiveRefCntPtr<Value>;

  CodeCompletionCache(OnDiskCodeCompletionCache *nextCache = nullptr);
  ~CodeCompletionCache();

  static ValueRefCntPtr createValue();
  std::optional<ValueRefCntPtr> get(const Key &K);
  void set(const Key &K, ValueRefCntPtr V) { setImpl(K, V, /*setChain*/ true); }

private:
  void setImpl(const Key &K, ValueRefCntPtr V, bool setChain);
};

/// On-disk per-module code completion result cache.
///
/// These results persist between multiple code completion requests and can be
/// used with different ASTContexts.
class OnDiskCodeCompletionCache {
  std::string cacheDirectory;

public:
  using Key = CodeCompletionCache::Key;
  using Value = CodeCompletionCache::Value;
  using ValueRefCntPtr = CodeCompletionCache::ValueRefCntPtr;

  OnDiskCodeCompletionCache(Twine cacheDirectory);
  ~OnDiskCodeCompletionCache();

  std::optional<ValueRefCntPtr> get(const Key &K);
  std::error_code set(const Key &K, ValueRefCntPtr V);

  static std::optional<ValueRefCntPtr> getFromFile(StringRef filename);
};

struct RequestedCachedModule {
  CodeCompletionCache::Key Key;
  const ModuleDecl *TheModule;
  CodeCompletionFilter Filter;
};

} // end namespace ide
} // end namespace language

namespace llvm {
template<>
struct DenseMapInfo<swift::ide::CodeCompletionCache::Key> {
  using KeyTy = swift::ide::CodeCompletionCache::Key;
  static inline KeyTy getEmptyKey() {
    return KeyTy{/*ModuleFilename=*/"",
                 /*ModuleName=*/"",
                 /*AccessPath=*/{},
                 /*ResultsHaveLeadingDot=*/false,
                 /*ForTestableLookup=*/false,
                 /*ForPrivateImportLookup=*/false,
                 /*SpiGroups=*/{},
                 /*AddInitsInToplevel=*/false,
                 /*AddCallWithNoDefaultArgs=*/false,
                 /*Annotated=*/false};
  }
  static inline KeyTy getTombstoneKey() {
    return KeyTy{/*ModuleFilename=*/"",
                 /*ModuleName=*/"",
                 /*AccessPath=*/{},
                 /*ResultsHaveLeadingDot=*/true,
                 /*ForTestableLookup=*/false,
                 /*ForPrivateImportLookup=*/false,
                 /*SpiGroups=*/{},
                 /*AddInitsInToplevel=*/false,
                 /*AddCallWithNoDefaultArgs=*/false,
                 /*Annotated=*/false};
  }
  static unsigned getHashValue(const KeyTy &Val) {
    return llvm::hash_combine(
        Val.ModuleFilename, Val.ModuleName,
        llvm::hash_combine_range(Val.AccessPath.begin(), Val.AccessPath.end()),
        Val.ResultsHaveLeadingDot, Val.ForTestableLookup,
        llvm::hash_combine_range(Val.SpiGroups.begin(), Val.SpiGroups.end()),
        Val.ForPrivateImportLookup, Val.AddInitsInToplevel,
        Val.AddCallWithNoDefaultArgs, Val.Annotated);
  }
  static bool isEqual(const KeyTy &LHS, const KeyTy &RHS) {
    return LHS == RHS;
  }
};
} // end namespace llvm

#endif // SWIFT_IDE_CODE_COMPLETIONCACHE_H
