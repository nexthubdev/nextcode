//===--- SwiftObject.h - Native Swift Object root class ---------*- C++ -*-===//
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
// This implements the Objective-C root class that provides basic `id`-
// compatibility and `NSObject` protocol conformance for pure Swift classes.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_RUNTIME_SWIFTOBJECT_H
#define SWIFT_RUNTIME_SWIFTOBJECT_H

#include "language/Runtime/Config.h"
#include <cstdint>
#include <utility>
#include "language/Runtime/HeapObject.h"
#include "../runtime/SwiftHashableSupport.h"
#if SWIFT_OBJC_INTEROP
#include <objc/NSObject.h>
#endif


#if SWIFT_OBJC_INTEROP
#if __OBJC__

// Source code: "SwiftObject"
// Real class name: mangled "Swift._SwiftObject"
#define SwiftObject _TtCs12_SwiftObject

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-root-class"
#if __has_attribute(objc_root_class)
__attribute__((__objc_root_class__))
#endif
SWIFT_RUNTIME_EXPORT @interface SwiftObject<NSObject> {
 @private
  Class isa;
  SWIFT_HEAPOBJECT_NON_OBJC_MEMBERS;
}
#pragma clang diagnostic pop

- (BOOL)isEqual:(id)object;
- (NSUInteger)hash;

- (Class)superclass;
- (Class)class;
- (instancetype)self;
- (struct _NSZone *)zone;

- (id)performSelector:(SEL)aSelector;
- (id)performSelector:(SEL)aSelector withObject:(id)object;
- (id)performSelector:(SEL)aSelector withObject:(id)object1 withObject:(id)object2;

- (BOOL)isProxy;

+ (BOOL)isSubclassOfClass:(Class)aClass;
- (BOOL)isKindOfClass:(Class)aClass;
- (BOOL)isMemberOfClass:(Class)aClass;
- (BOOL)conformsToProtocol:(Protocol *)aProtocol;

- (BOOL)respondsToSelector:(SEL)aSelector;
+ (BOOL)instancesRespondToSelector:(SEL)aSelector;
- (IMP)methodForSelector:(SEL)aSelector;
+ (IMP)instanceMethodForSelector:(SEL)aSelector;

- (instancetype)retain;
- (oneway void)release;
- (instancetype)autorelease;
- (NSUInteger)retainCount;

- (id /* NSString */)description;
- (id /* NSString */)debugDescription;
@end

namespace language {

id getDescription(OpaqueValue *value, const Metadata *type);

}

#endif
#endif

namespace language {

/// Get the NSObject metadata.
const Metadata *getNSObjectMetadata();
const Metadata *getNSStringMetadata();

namespace hashable_support {
const HashableWitnessTable *getNSStringHashableConformance();
}
}

#endif
