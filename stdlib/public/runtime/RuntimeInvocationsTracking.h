//===--- RuntimeInvocationsTracking.h ---------------------------*- C++ -*-===//
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
// Track invocations of Swift runtime functions. This can be used for
// performance analysis.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_STDLIB_RUNTIME_INVOCATIONS_TRACKING_H
#define SWIFT_STDLIB_RUNTIME_INVOCATIONS_TRACKING_H

#include "language/Runtime/Config.h"

/// This API is only enabled if this define is set.
#if defined(SWIFT_ENABLE_RUNTIME_FUNCTION_COUNTERS)

#if defined(__cplusplus)

namespace language {
struct HeapObject;
struct RuntimeFunctionCountersState;
} // end namespace language

using namespace language;
#else

struct HeapObject;
struct RuntimeFunctionCountersState;

#endif

/// The name of a helper function for tracking the calls of a runtime function.
#define SWIFT_RT_TRACK_INVOCATION_NAME(RT_FUNCTION)                            \
  swift_trackRuntimeInvocation_##RT_FUNCTION

/// Invoke a helper function for tracking the calls of a runtime function.
#define SWIFT_RT_TRACK_INVOCATION(OBJ, RT_FUNCTION)                            \
  SWIFT_RT_TRACK_INVOCATION_NAME(RT_FUNCTION)(OBJ)

#define FUNCTION_TO_TRACK(RT_FUNCTION)                                         \
  extern void SWIFT_RT_TRACK_INVOCATION_NAME(RT_FUNCTION)(HeapObject * OBJ);
/// Declarations of external functions for invocations tracking.
#include "RuntimeInvocationsTracking.def"

/// This type defines a callback to be called on any intercepted runtime
/// function.
using RuntimeFunctionCountersUpdateHandler =
  __attribute__((swiftcall))
  void (*)(HeapObject *object, int64_t runtimeFunctionID);

/// Public APIs

/// Get the runtime object state associated with an object and store it
/// into the result.
SWIFT_RUNTIME_EXPORT void
_swift_getObjectRuntimeFunctionCounters(HeapObject *object,
                                        RuntimeFunctionCountersState *result);

/// Get the global runtime state containing the total numbers of invocations for
/// each runtime function of interest and store it into the result.
SWIFT_RUNTIME_EXPORT void _swift_getGlobalRuntimeFunctionCounters(
    swift::RuntimeFunctionCountersState *result);

/// Return the names of the runtime functions being tracked.
/// Their order is the same as the order of the counters in the
/// RuntimeObjectState structure.
SWIFT_RUNTIME_EXPORT const char **_swift_getRuntimeFunctionNames();

/// Return the offsets of the runtime function counters being tracked.
/// Their order is the same as the order of the counters in the
/// RuntimeFunctionCountersState structure.
SWIFT_RUNTIME_EXPORT const uint16_t *_swift_getRuntimeFunctionCountersOffsets();

/// Return the number of runtime functions being tracked.
SWIFT_RUNTIME_EXPORT uint64_t _swift_getNumRuntimeFunctionCounters();

/// Dump all per-object runtime function pointers.
SWIFT_RUNTIME_EXPORT void _swift_dumpObjectsRuntimeFunctionPointers();

/// Set mode for global runtime function counters.
/// Return the old value of this flag.
SWIFT_RUNTIME_EXPORT int
_swift_setPerObjectRuntimeFunctionCountersMode(int mode);

/// Set mode for per object runtime function counters.
/// Return the old value of this flag.
SWIFT_RUNTIME_EXPORT int _swift_setGlobalRuntimeFunctionCountersMode(int mode);

/// Set the global runtime state of function pointers from a provided state.
SWIFT_RUNTIME_EXPORT void _swift_setGlobalRuntimeFunctionCounters(
    swift::RuntimeFunctionCountersState *state);

/// Set the runtime object state associated with an object from a provided
/// state.
SWIFT_RUNTIME_EXPORT void
_swift_setObjectRuntimeFunctionCounters(HeapObject *object,
                                        RuntimeFunctionCountersState *state);

/// Set the global runtime function counters update handler.
SWIFT_RUNTIME_EXPORT RuntimeFunctionCountersUpdateHandler
_swift_setGlobalRuntimeFunctionCountersUpdateHandler(
    RuntimeFunctionCountersUpdateHandler handler);

#else

/// Let runtime functions unconditionally use this define by making it a NOP if
/// counters are not enabled.
#define SWIFT_RT_TRACK_INVOCATION(OBJ, RT_FUNCTION)

#endif // SWIFT_ENABLE_RUNTIME_FUNCTION_COUNTERS

#endif
