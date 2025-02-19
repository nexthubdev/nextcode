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

package modtest

import (
	"os"
	"testing"

	"github.com/goplus/mod/modload"
)

func LoadFrom(gomod, gopmod string, gomodText, gopmodText string) (mod modload.Module, err error) {
	return modload.LoadFromEx(gomod, gopmod, func(s string) ([]byte, error) {
		if s == gomod {
			return []byte(gomodText), nil
		} else if s == gopmod && gopmodText != "" {
			return []byte(gopmodText), nil
		}
		return nil, os.ErrNotExist
	})
}

func Load(t *testing.T, gomodText, gopmodText string, errMsg string) modload.Module {
	mod, err := LoadFrom("/foo/go.mod", "/foo/gop.mod", gomodText, gopmodText)
	if err != nil {
		if err.Error() != errMsg {
			t.Fatal("LoadFrom:", err)
		}
	}
	return mod
}

func GopCommunity(t *testing.T) modload.Module {
	const gomodText = `
module github.com/goplus/community

go 1.18

require (
	github.com/goplus/yap v0.5.0 //gop:class
)
`
	return Load(t, gomodText, ``, ``)
}

func GopClass(t *testing.T) modload.Module {
	const gomodText = `
module github.com/goplus/community

go 1.18

require (
	github.com/goplus/yap v0.5.0 //gop:class
	github.com/qiniu/a v0.1.0
	github.com/qiniu/x v1.13.2 // gop:class
)
`
	mod := Load(t, gomodText, ``, ``)
	if n := len(mod.Opt.ClassMods); n != 2 {
		t.Fatal("len(mod.Opt.Import):", n)
	}
	return mod
}

func LLGoCompiler(t *testing.T) modload.Module {
	const gomodText = `
module github.com/goplus/llgo

go 1.18 // llgo 0.9
`
	mod := Load(t, gomodText, ``, ``)
	if cl := mod.Opt.Compiler; cl == nil || cl.Name != "llgo" || cl.Version != "0.9" {
		t.Fatal("mod.Opt.Compiler:", cl)
	}
	return mod
}

func TinyGoCompiler(t *testing.T) modload.Module {
	const gomodText = `
module github.com/tinygo-org/tinygo

go 1.18 // tinygo 0.32
`
	mod := Load(t, gomodText, ``, ``)
	if cl := mod.Opt.Compiler; cl == nil || cl.Name != "tinygo" || cl.Version != "0.32" {
		t.Fatal("mod.Opt.Compiler:", cl)
	}
	return mod
}

func Import(t *testing.T) modload.Module {
	const gomodText = `
module github.com/goplus/yap

go 1.18
`
	const gopmodText = `
gop 1.2

project _yap.gox App github.com/goplus/yap

project _ytest.gox App github.com/goplus/yap/test
class _ytest.gox Case
import github.com/goplus/yap/ytest/auth/jwt
import yauth github.com/goplus/yap/ytest/auth
`
	mod := Load(t, gomodText, gopmodText, ``)
	if n := len(mod.Opt.Projects); n != 2 {
		t.Fatal("len(mod.Opt.Projects):", n)
	}
	return mod
}
