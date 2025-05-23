//===--- InstrumentsSupport.h - Support for Instruments.app -----*- C++ -*-===//
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
// Swift runtime support for instruments.app
// In the long run, they plan to use dyld to make this indirection go away.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_RUNTIME_INSTRUMENTS_SUPPORT_H
#define SWIFT_RUNTIME_INSTRUMENTS_SUPPORT_H

#define SWIFT_RT_DECLARE_ENTRY \
  __ptrauth_swift_runtime_function_entry

namespace language {

#ifdef SWIFT_STDLIB_OVERRIDABLE_RETAIN_RELEASE

// liboainject patches the function pointers and calls the functions below.
SWIFT_RUNTIME_EXPORT
HeapObject *(*SWIFT_RT_DECLARE_ENTRY _swift_allocObject)(
                                  HeapMetadata const *metadata,
                                  size_t requiredSize,
                                  size_t requiredAlignmentMask);
SWIFT_RUNTIME_EXPORT
HeapObject *(*SWIFT_RT_DECLARE_ENTRY _swift_retain)(HeapObject *object);
SWIFT_RUNTIME_EXPORT
HeapObject *(*SWIFT_RT_DECLARE_ENTRY _swift_retain_n)(HeapObject *object, uint32_t n);
SWIFT_RUNTIME_EXPORT
HeapObject *(*SWIFT_RT_DECLARE_ENTRY _swift_tryRetain)(HeapObject *object);
SWIFT_RUNTIME_EXPORT
void (*SWIFT_RT_DECLARE_ENTRY _swift_release)(HeapObject *object);
SWIFT_RUNTIME_EXPORT
void (*SWIFT_RT_DECLARE_ENTRY _swift_release_n)(HeapObject *object, uint32_t n);
SWIFT_RUNTIME_EXPORT
std::atomic<bool> _swift_enableSwizzlingOfAllocationAndRefCountingFunctions_forInstrumentsOnly;
SWIFT_RUNTIME_EXPORT
size_t swift_retainCount(HeapObject *object);

// liboainject tries to patch the function pointers and call the functions below
// Swift used to implement these but no longer does.
// Do not reuse these names unless you do what oainject expects you to do.
SWIFT_RUNTIME_EXPORT
void *(*_swift_alloc)(size_t idx);
SWIFT_RUNTIME_EXPORT
void *(*_swift_tryAlloc)(size_t idx);
SWIFT_RUNTIME_EXPORT
void *(*_swift_slowAlloc)(size_t bytes, size_t alignMask, uintptr_t flags);
SWIFT_RUNTIME_EXPORT
void (*_swift_dealloc)(void *ptr, size_t idx);
SWIFT_RUNTIME_EXPORT
void (*_swift_slowDealloc)(void *ptr, size_t bytes, size_t alignMask);
SWIFT_RUNTIME_EXPORT
size_t _swift_indexToSize(size_t idx);
SWIFT_RUNTIME_EXPORT
void _swift_zone_init(void);

#endif // SWIFT_STDLIB_OVERRIDABLE_RETAIN_RELEASE

}

#endif
