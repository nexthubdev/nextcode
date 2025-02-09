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

package parser

import (
	"bytes"
	"os"
	"os/exec"
	"strings"

	"github.com/goplus/llgo/xtool/clang/ast"
	jsoniter "github.com/json-iterator/go"
)

type Mode uint

// -----------------------------------------------------------------------------

type ParseError struct {
	Err    error
	Stderr []byte
}

func (p *ParseError) Error() string {
	if len(p.Stderr) > 0 {
		return string(p.Stderr)
	}
	return p.Err.Error()
}

// -----------------------------------------------------------------------------

type Config struct {
	Json   *[]byte
	Flags  []string
	Stderr bool
}

func DumpAST(filename string, conf *Config) (result []byte, warning []byte, err error) {
	if conf == nil {
		conf = new(Config)
	}
	skiperr := strings.HasSuffix(filename, "vfprintf.c.i")
	stdout := NewPagedWriter()
	stderr := new(bytes.Buffer)
	args := []string{"-Xclang", "-ast-dump=json", "-fsyntax-only", filename}
	if len(conf.Flags) != 0 {
		args = append(conf.Flags, args...)
	}
	cmd := exec.Command("clang", args...)
	cmd.Stdin = os.Stdin
	cmd.Stdout = stdout
	if conf.Stderr && !skiperr {
		cmd.Stderr = os.Stderr
	} else {
		cmd.Stderr = stderr
	}
	err = cmd.Run()
	errmsg := stderr.Bytes()
	if err != nil && !skiperr {
		return nil, nil, &ParseError{Err: err, Stderr: errmsg}
	}
	return stdout.Bytes(), errmsg, nil
}

// -----------------------------------------------------------------------------

var json = jsoniter.ConfigCompatibleWithStandardLibrary

func ParseFileEx(filename string, mode Mode, conf *Config) (file *ast.Node, warning []byte, err error) {
	out, warning, err := DumpAST(filename, conf)
	if err != nil {
		return
	}
	if conf != nil && conf.Json != nil {
		*conf.Json = out
	}
	file = new(ast.Node)
	err = json.Unmarshal(out, file)
	if err != nil {
		err = &ParseError{Err: err}
	}
	return
}

func ParseFile(filename string, mode Mode) (file *ast.Node, warning []byte, err error) {
	return ParseFileEx(filename, mode, nil)
}

// -----------------------------------------------------------------------------
