#[[
This source file is part of the Swift open source project

Copyright (c) 2014 - 2025 Apple Inc. and the Swift project authors
Licensed under Apache License v2.0 with Runtime Library Exception

See http://swift.org/LICENSE.txt for license information
See http://swift.org/CONTRIBUTORS.txt for Swift project authors
]]

add_library(SWBLibc
  libc.swift)
set_target_properties(SWBLibc PROPERTIES
  Swift_LANGUAGE_VERSION 6)
target_link_libraries(SWBLibc PUBLIC
  SWBCLibc)

# FIXME: why is this needed? We should be wiring up the Swift_MODULE_DIRECTORY
# without this explicit configuration.
target_include_directories(SWBLibc PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR})
