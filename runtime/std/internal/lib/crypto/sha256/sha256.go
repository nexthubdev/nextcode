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

package sha256

// llgo:skipall
import (
	"crypto"
	"hash"
	"unsafe"

	c "github.com/goplus/llgo/runtime/internal/clite"
	"github.com/goplus/llgo/runtime/internal/clite/openssl"
)

func init() {
	crypto.RegisterHash(crypto.SHA224, New224)
	crypto.RegisterHash(crypto.SHA256, New)
}

// The blocksize of SHA256 and SHA224 in bytes.
const BlockSize = 64

// The size of a SHA256 checksum in bytes.
const Size = 32

type digest256 struct {
	ctx openssl.SHA256_CTX
}

func (d *digest256) Size() int { return Size }

func (d *digest256) BlockSize() int { return BlockSize }

func (d *digest256) Reset() {
	d.ctx.Init()
}

func (d *digest256) Write(p []byte) (nn int, err error) {
	d.ctx.UpdateBytes(p)
	return len(p), nil
}

func (d *digest256) Sum(in []byte) []byte {
	hash := (*[Size]byte)(c.Alloca(Size))
	d.ctx.Final((*byte)(unsafe.Pointer(hash)))
	return append(in, hash[:]...)
}

// New returns a new hash.Hash computing the SHA256 checksum.
func New() hash.Hash {
	d := new(digest256)
	d.ctx.Init()
	return d
}

// Sum256 returns the SHA256 checksum of the data.
func Sum256(data []byte) (ret [Size]byte) {
	openssl.SHA256Bytes(data, &ret[0])
	return
}
