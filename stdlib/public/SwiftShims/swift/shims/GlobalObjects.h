//===--- GlobalObjects.h - Statically-initialized objects -------*- C++ -*-===//
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
#ifndef SWIFT_STDLIB_SHIMS_GLOBALOBJECTS_H_
#define SWIFT_STDLIB_SHIMS_GLOBALOBJECTS_H_

#include "languageStdint.h"
#include "languageStdbool.h"
#include "HeapObject.h"
#include "Visibility.h"

#ifdef __cplusplus
#ifndef __swift__
namespace language {
#endif
extern "C" {
#endif

struct _SwiftArrayBodyStorage {
  __swift_intptr_t count;
  __swift_uintptr_t _capacityAndFlags;
};

struct _SwiftEmptyArrayStorage {
  struct HeapObject header;
  struct _SwiftArrayBodyStorage body;
};

SWIFT_RUNTIME_STDLIB_API
struct _SwiftEmptyArrayStorage _swiftEmptyArrayStorage;

struct _SwiftDictionaryBodyStorage {
  __swift_intptr_t count;
  __swift_intptr_t capacity;
  __swift_int8_t scale;
  __swift_int8_t reservedScale;
  __swift_int16_t extra;
  __swift_int32_t age;
  __swift_intptr_t seed;
  void *rawKeys;
  void *rawValues;
};

struct _SwiftSetBodyStorage {
  __swift_intptr_t count;
  __swift_intptr_t capacity;
  __swift_int8_t scale;
  __swift_int8_t reservedScale;
  __swift_int16_t extra;
  __swift_int32_t age;
  __swift_intptr_t seed;
  void *rawElements;
};

struct _SwiftEmptyDictionarySingleton {
  struct HeapObject header;
  struct _SwiftDictionaryBodyStorage body;
  __swift_uintptr_t metadata;
};

struct _SwiftEmptySetSingleton {
  struct HeapObject header;
  struct _SwiftSetBodyStorage body;
  __swift_uintptr_t metadata;
};

SWIFT_RUNTIME_STDLIB_API
struct _SwiftEmptyDictionarySingleton _swiftEmptyDictionarySingleton;

SWIFT_RUNTIME_STDLIB_API
struct _SwiftEmptySetSingleton _swiftEmptySetSingleton;

struct _SwiftHashingParameters {
  __swift_uint64_t seed0;
  __swift_uint64_t seed1;
  __swift_bool deterministic;
};
  
SWIFT_RUNTIME_STDLIB_API
struct _SwiftHashingParameters _swift_stdlib_Hashing_parameters;

#ifdef __cplusplus

static_assert(
  sizeof(_SwiftDictionaryBodyStorage) ==
    5 * sizeof(__swift_intptr_t) + sizeof(__swift_int64_t),
  "_SwiftDictionaryBodyStorage has unexpected size");

static_assert(
  sizeof(_SwiftSetBodyStorage) ==
    4 * sizeof(__swift_intptr_t) + sizeof(__swift_int64_t),
  "_SwiftSetBodyStorage has unexpected size");

} // extern "C"
#ifndef __swift__
} // namespace language
#endif
#endif

#endif
