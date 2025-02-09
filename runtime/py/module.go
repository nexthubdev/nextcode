/*
 * Copyright (c) NeXTHub Corporation. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 */

package py

import (
	_ "unsafe"

	"github.com/goplus/llgo/c"
)

// https://docs.python.org/3/c-api/import.html
// https://docs.python.org/3/c-api/module.html

/*
// llgo:type C
type ModuleDefBase struct {
	Unused [8]byte // TODO(xsw)
}

// llgo:type C
type ModuleDef struct {
	Base ModuleDefBase
	// TODO(xsw)
}
*/

// Return the module object corresponding to a module name. The name argument
// may be of the form package.module. First check the modules dictionary if
// there’s one there, and if not, create a new one and insert it in the modules
// dictionary. Return nil with an exception set on failure.
//
//go:linkname AddModule C.PyImport_AddModule
func AddModule(name *c.Char) *Object

// This is a wrapper around py.Import which takes a const char* as an argument
// instead of an Object.
//
//go:linkname ImportModule C.PyImport_ImportModule
func ImportModule(name *c.Char) *Object

// This is a higher-level interface that calls the current “import hook function” (with
// an explicit level of 0, meaning absolute import). It invokes the __import__() function
// from the __builtins__ of the current globals. This means that the import is done using
// whatever import hooks are installed in the current environment.
//
// This function always uses absolute imports.
//
//go:linkname Import C.PyImport_Import
func Import(name *Object) *Object

// Return the dictionary object that implements module’s namespace; this object is the same
// as the __dict__ attribute of the module object. If module is not a module object (or a
// subtype of a module object), SystemError is raised and nil is returned.
//
// It is recommended extensions use other Module and Object functions rather than directly
// manipulate a module’s __dict__.
//
// llgo:link (*Object).ModuleGetDict C.PyModule_GetDict
func (m *Object) ModuleGetDict() *Object { return nil }

// llgo:link (*Object).ModuleLoadSyms C.llgoLoadPyModSyms
func (m *Object) ModuleLoadSyms(__llgo_va_list ...any) {}

// -----------------------------------------------------------------------------
