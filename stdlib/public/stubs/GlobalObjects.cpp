//===--- GlobalObjects.cpp - Statically-initialized objects ---------------===//
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
//  Objects that are allocated at global scope instead of on the heap,
//  and statically initialized to avoid synchronization costs, are
//  defined here.
//
//===----------------------------------------------------------------------===//

#include "language/shims/GlobalObjects.h"
#include "language/shims/Random.h"
#include "language/Runtime/Metadata.h"
#include "language/Runtime/Debug.h"
#include "language/Runtime/EnvironmentVariables.h"
#include <stdlib.h>

namespace language {
// FIXME(ABI)#76 : does this declaration need SWIFT_RUNTIME_STDLIB_API?
// _direct type metadata for Swift.__EmptyArrayStorage
SWIFT_RUNTIME_STDLIB_API
ClassMetadata CLASS_METADATA_SYM(s19__EmptyArrayStorage);

// _direct type metadata for Swift.__EmptyDictionarySingleton
SWIFT_RUNTIME_STDLIB_API
ClassMetadata CLASS_METADATA_SYM(s26__EmptyDictionarySingleton);

// _direct type metadata for Swift.__EmptySetSingleton
SWIFT_RUNTIME_STDLIB_API
ClassMetadata CLASS_METADATA_SYM(s19__EmptySetSingleton);
} // namespace language

SWIFT_RUNTIME_STDLIB_API
swift::_SwiftEmptyArrayStorage swift::_swiftEmptyArrayStorage = {
  // HeapObject header;
  {
    &swift::CLASS_METADATA_SYM(s19__EmptyArrayStorage), // isa pointer
    InlineRefCounts::Immortal
  },
  
  // _SwiftArrayBodyStorage body;
  {
    0, // int count;                                    
    1  // unsigned int _capacityAndFlags; 1 means elementTypeIsBridgedVerbatim
  }
};

// Define `__swiftImmortalRefCount` which is used by constant static arrays.
// It is the bit pattern for the ref-count field of the array buffer.
//
// TODO: Support constant static arrays on other platforms, too.
// This needs a bit more work because the tricks with absolute symbols and
// symbol aliases don't work this way with other object file formats than Mach-O.
#if defined(__APPLE__)

__asm__("  .globl __swiftImmortalRefCount\n");

#if __POINTER_WIDTH__ == 64

  // TODO: is there a way to avoid hard coding this constant in the inline
  //       assembly string?
  static_assert(swift::InlineRefCountBits::immortalBits() == 0x80000004ffffffffull,
                "immortal refcount bits changed: correct the inline asm below");
  __asm__(".set __swiftImmortalRefCount, 0x80000004ffffffff\n");

#elif __POINTER_WIDTH__ == 32

  // TODO: is there a way to avoid hard coding this constant in the inline
  //       assembly string?
  static_assert(swift::InlineRefCountBits::immortalBits() == 0x800004fful,
                "immortal refcount bits changed: correct the inline asm below");
  __asm__(".set __swiftImmortalRefCount, 0x800004ff\n");

#else
  #error("unsupported pointer width")
#endif

#endif

SWIFT_RUNTIME_STDLIB_API
swift::_SwiftEmptyDictionarySingleton swift::_swiftEmptyDictionarySingleton = {
  // HeapObject header;
  {
    &swift::CLASS_METADATA_SYM(s26__EmptyDictionarySingleton), // isa pointer
    InlineRefCounts::Immortal
  },
  
  // _SwiftDictionaryBodyStorage body;
  {
    // Setting the scale to 0 makes for a bucketCount of 1 -- so that the
    // storage consists of a single unoccupied bucket. The capacity is set to
    // 0 so that any insertion will lead to real storage being allocated.
    0, // int count;
    0, // int capacity;                                    
    0, // int8 scale;
    0, // int8 reservedScale;
    0, // int16 extra;
    0, // int32 age;
    0, // int seed;
    (void *)1, // void* keys; (non-null garbage)
    (void *)1  // void* values; (non-null garbage)
  },

  // bucket 0 is unoccupied; other buckets are out-of-bounds
  static_cast<__swift_uintptr_t>(~1) // int metadata; 
};

SWIFT_RUNTIME_STDLIB_API
swift::_SwiftEmptySetSingleton swift::_swiftEmptySetSingleton = {
  // HeapObject header;
  {
    &swift::CLASS_METADATA_SYM(s19__EmptySetSingleton), // isa pointer
    InlineRefCounts::Immortal
  },
  
  // _SwiftSetBodyStorage body;
  {
    // Setting the scale to 0 makes for a bucketCount of 1 -- so that the
    // storage consists of a single unoccupied bucket. The capacity is set to
    // 0 so that any insertion will lead to real storage being allocated.
    0, // int count;
    0, // int capacity;                                    
    0, // int8 scale;
    0, // int8 reservedScale;
    0, // int16 extra;
    0, // int32 age;
    0, // int seed;
    (void *)1, // void *rawElements; (non-null garbage)
  },

  // bucket 0 is unoccupied; other buckets are out-of-bounds
  static_cast<__swift_uintptr_t>(~1) // int metadata;
};

static swift::_SwiftHashingParameters initializeHashingParameters() {
  // Setting the environment variable SWIFT_DETERMINISTIC_HASHING to "1"
  // disables randomized hash seeding. This is useful in cases we need to ensure
  // results are repeatable, e.g., in certain test environments.  (Note that
  // even if the seed override is enabled, hash values aren't guaranteed to
  // remain stable across even minor stdlib releases.)
  if (swift::runtime::environment::SWIFT_DETERMINISTIC_HASHING()) {
    return { 0, 0, true };
  }
  __swift_uint64_t seed0 = 0, seed1 = 0;
  swift_stdlib_random(&seed0, sizeof(seed0));
  swift_stdlib_random(&seed1, sizeof(seed1));
  return { seed0, seed1, false };
}

SWIFT_ALLOWED_RUNTIME_GLOBAL_CTOR_BEGIN
swift::_SwiftHashingParameters swift::_swift_stdlib_Hashing_parameters =
  initializeHashingParameters();
SWIFT_ALLOWED_RUNTIME_GLOBAL_CTOR_END


SWIFT_RUNTIME_STDLIB_API
void swift::_swift_instantiateInertHeapObject(void *address,
                                              const HeapMetadata *metadata) {
  ::new (address) HeapObject{metadata};
}
