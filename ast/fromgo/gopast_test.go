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

package fromgo

import (
	"bytes"
	"go/ast"
	"go/parser"
	"go/token"
	"testing"

	gopast "github.com/goplus/gop/ast"
	"github.com/goplus/gop/format"
)

func testAST(t *testing.T, from, to string) {
	fset := token.NewFileSet()
	f, err := parser.ParseFile(fset, "foo.go", from, 0)
	if err != nil {
		t.Fatal("parser.ParseFile:", err)
	}
	gopf := ASTFile(f, 0)
	var b bytes.Buffer
	err = format.Node(&b, fset, gopf)
	if err != nil {
		t.Fatal("format.Node:", err)
	}
	result := b.String()
	if to != result {
		t.Fatalf("\nResult:\n%s\nExpected:\n%s\n", result, to)
	}
}

func test(t *testing.T, src string) {
	testAST(t, src, src)
}

func testPanic(t *testing.T, panicMsg string, doPanic func()) {
	t.Run(panicMsg, func(t *testing.T) {
		defer func() {
			if e := recover(); e == nil {
				t.Fatal("testPanic: no error?")
			} else if msg := e.(string); msg != panicMsg {
				t.Fatalf("\nResult:\n%s\nExpected Panic:\n%s\n", msg, panicMsg)
			}
		}()
		doPanic()
	})
}

func TestErrASTFile(t *testing.T) {
	testPanic(t, "ASTFile: doesn't support keeping cgo now\n", func() {
		ASTFile(nil, KeepCgo)
	})
	testPanic(t, "ASTFile: doesn't support keeping func body now\n", func() {
		ASTFile(nil, KeepFuncBody)
	})
}

func TestErrDecl(t *testing.T) {
	testPanic(t, "gopDecl: unkown decl - <nil>\n", func() {
		gopDecl(nil)
	})
	testPanic(t, "gopGenDecl: unknown spec - ILLEGAL\n", func() {
		gopGenDecl(&ast.GenDecl{
			Specs: []ast.Spec{nil},
		})
	})
}

func TestErrExpr(t *testing.T) {
	testPanic(t, "gopExpr: unknown expr - *ast.BadExpr\n", func() {
		gopExpr(&ast.BadExpr{})
	})
}

func TestBasic(t *testing.T) {
	test(t, `package main

import "fmt"

type a struct {
	v   map[int]chan int
	arr *[2]func()
	i   interface{}
}

var b = &a{
	arr: &[2]func(){
		nil,
		func() {},
	},
}

const c = (10 + 20) * 2

var d = b.arr[1]

var e = b.arr[:1]

var f = a.i.(func() (int))()

func foo(v ...interface{}) {}
`)
}

func TestMethod(t *testing.T) {
	test(t, `package main

type foo int

func (a foo) Str() (string) {}
`)
}

func TestCheckIdent(t *testing.T) {
	if _, ok := CheckIdent(&gopast.Ident{}); ok {
		t.Fatal("CheckIdent: found?")
	}
	if _, ok := CheckIdent(&gopast.Ident{Obj: &gopast.Object{
		Data: &ast.Ident{},
	}}); !ok {
		t.Fatal("CheckIdent: not found?")
	}
}
