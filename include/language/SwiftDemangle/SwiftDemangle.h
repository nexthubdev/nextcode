//===--- SwiftDemangle.h - Public demangling interface ----------*- C++ -*-===//
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
///
/// \file
/// This header declares functions in the libswiftDemangle library,
/// which provides external access to Swift's demangler.
///
//===----------------------------------------------------------------------===//

#ifndef SWIFT_DEMANGLE_SWIFT_DEMANGLE_H
#define SWIFT_DEMANGLE_SWIFT_DEMANGLE_H

#include <swift/SwiftDemangle/Platform.h>

/// @{
/// Version constants for libswiftDemangle library.

/// Major version changes when there are ABI or source incompatible changes.
#define SWIFT_DEMANGLE_VERSION_MAJOR 1

/// Minor version changes when new APIs are added in ABI- and source-compatible
/// way.
#define SWIFT_DEMANGLE_VERSION_MINOR 2

/// @}

#ifdef __cplusplus
extern "C" {
#endif

/// Demangle Swift function names.
///
/// \returns the length of the demangled function name (even if greater than the
/// size of the output buffer) or 0 if the input is not a Swift-mangled function
/// name (in which cases \p OutputBuffer is left untouched).
SWIFT_DEMANGLE_LINKAGE
size_t swift_demangle_getDemangledName(const char *MangledName,
                                       char *OutputBuffer, size_t Length);

/// Demangle Swift function names with module names and implicit self
/// and metatype type names in function signatures stripped.
///
/// \returns the length of the demangled function name (even if greater than the
/// size of the output buffer) or 0 if the input is not a Swift-mangled function
/// name (in which cases \p OutputBuffer is left untouched).
SWIFT_DEMANGLE_LINKAGE
size_t swift_demangle_getSimplifiedDemangledName(const char *MangledName,
                                                 char *OutputBuffer,
                                                 size_t Length);

/// Demangle a Swift symbol and return the module name of the mangled entity.
///
/// \returns the length of the demangled module name (even if greater than the
/// size of the output buffer) or 0 if the input is not a Swift-mangled name
/// (in which cases \p OutputBuffer is left untouched).
SWIFT_DEMANGLE_LINKAGE
size_t swift_demangle_getModuleName(const char *MangledName,
                                    char *OutputBuffer,
                                    size_t Length);

/// Demangles a Swift function name and returns true if the function
/// conforms to the Swift calling convention.
///
/// \returns true if the function conforms to the Swift calling convention.
/// The return value is unspecified if the \p MangledName does not refer to a
/// function symbol.
SWIFT_DEMANGLE_LINKAGE
int swift_demangle_hasSwiftCallingConvention(const char *MangledName);

#ifdef __cplusplus
} // extern "C"
#endif

// Old API.  To be removed when we remove the compatibility symlink.

/// @{
/// Version constants for libfunctionNameDemangle library.

/// Major version changes when there are ABI or source incompatible changes.
#define FUNCTION_NAME_DEMANGLE_VERSION_MAJOR 0

/// Minor version changes when new APIs are added in ABI- and source-compatible
/// way.
#define FUNCTION_NAME_DEMANGLE_VERSION_MINOR 2

/// @}

#ifdef __cplusplus
extern "C" {
#endif

/// Demangle Swift function names.
///
/// Note that this function has a generic name because it is called from
/// contexts where it is not appropriate to use code names.
///
/// \returns the length of the demangled function name (even if greater than the
/// size of the output buffer) or 0 if the input is not a Swift-mangled function
/// name (in which cases \p OutputBuffer is left untouched).
SWIFT_DEMANGLE_LINKAGE
size_t fnd_get_demangled_name(const char *MangledName, char *OutputBuffer,
                              size_t Length);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SWIFT_DEMANGLE_SWIFT_DEMANGLE_H

