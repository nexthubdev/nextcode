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

package rand

// llgo:skipall
import (
	"io"

	c "github.com/goplus/llgo/runtime/internal/clite"
	"github.com/goplus/llgo/runtime/internal/clite/openssl"
	"github.com/qiniu/x/errors"
)

type rndReader struct{}

func (rndReader) Read(p []byte) (n int, err error) {
	return Read(p)
}

// Reader is a global, shared instance of a cryptographically
// secure random number generator.
var Reader io.Reader = rndReader{}

type opensslError struct {
	file     *c.Char
	line     c.Int
	flags    c.Int
	function *c.Char
	data     *c.Char
	err      openssl.Errno
}

// [error code]:[library name]:[function name]:[reason string]:[[filename:line]]: [text message]
func (p *opensslError) Error() string {
	const bufsize = 1024
	buf := (*c.Char)(c.Alloca(bufsize))
	lib := openssl.ERRLibErrorString(p.err)
	reason := openssl.ERRReasonErrorString(p.err)
	n := uintptr(c.Snprintf(
		buf, bufsize,
		c.Str("%d:%s:%s:%s:[%s:%d]: "),
		p.err, lib, p.function, reason, p.file, p.line))
	n += c.Strlen(openssl.ERRErrorString(p.err, c.Advance(buf, n)))
	return c.GoString(buf, n)
}

func getError() *opensslError {
	ret := new(opensslError)
	err := openssl.ERRGetErrorAll(&ret.file, &ret.line, &ret.function, &ret.data, &ret.flags)
	if err == 0 {
		return nil
	}
	return ret
}

func getErrors() error {
	var errs errors.List
	for openssl.ERRPeekError() != 0 {
		errs.Add(getError())
	}
	return errs.ToError()
}

// Read is a helper function that calls Reader.Read using io.ReadFull.
// On return, n == len(b) if and only if err == nil.
func Read(b []byte) (n int, err error) {
	if openssl.RANDBytes(b) != 0 {
		return len(b), nil
	}
	return 0, getErrors()
}

/* TODO(xsw):
// batched returns a function that calls f to populate a []byte by chunking it
// into subslices of, at most, readMax bytes.
func batched(f func([]byte) error, readMax int) func([]byte) error {
	return func(out []byte) error {
		for len(out) > 0 {
			read := len(out)
			if read > readMax {
				read = readMax
			}
			if err := f(out[:read]); err != nil {
				return err
			}
			out = out[read:]
		}
		return nil
	}
}
*/
