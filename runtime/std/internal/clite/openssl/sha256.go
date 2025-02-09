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

package openssl

import (
	"unsafe"

	c "github.com/goplus/llgo/runtime/internal/clite"
)

type SHA256_CTX struct {
	H          [8]SHA_LONG
	Nl, Nh     SHA_LONG
	Data       [SHA_LBLOCK]SHA_LONG
	Num, MdLen c.Uint
}

type SHA224_CTX SHA256_CTX

// OSSL_DEPRECATEDIN_3_0 int SHA224_Init(SHA256_CTX *c);
//
// llgo:link (*SHA224_CTX).Init C.SHA224_Init
func (c *SHA224_CTX) Init() c.Int { return 0 }

// OSSL_DEPRECATEDIN_3_0 int SHA224_Update(SHA256_CTX *c, const void *data, size_t len);
//
// llgo:link (*SHA224_CTX).Update C.SHA224_Update
func (c *SHA224_CTX) Update(data unsafe.Pointer, n uintptr) c.Int { return 0 }

func (c *SHA224_CTX) UpdateBytes(data []byte) c.Int {
	return c.Update(unsafe.Pointer(unsafe.SliceData(data)), uintptr(len(data)))
}

func (c *SHA224_CTX) UpdateString(data string) c.Int {
	return c.Update(unsafe.Pointer(unsafe.StringData(data)), uintptr(len(data)))
}

// OSSL_DEPRECATEDIN_3_0 int SHA224_Final(unsigned char *md, SHA256_CTX *c);
//
//go:linkname sha224Final C.SHA224_Final
func sha224Final(md *byte, c *SHA224_CTX) c.Int

func (c *SHA224_CTX) Final(md *byte) c.Int {
	return sha224Final(md, c)
}

// OSSL_DEPRECATEDIN_3_0 int SHA256_Init(SHA256_CTX *c);
//
// llgo:link (*SHA256_CTX).Init C.SHA256_Init
func (c *SHA256_CTX) Init() c.Int { return 0 }

// OSSL_DEPRECATEDIN_3_0 int SHA256_Update(SHA256_CTX *c, const void *data, size_t len);
//
// llgo:link (*SHA256_CTX).Update C.SHA256_Update
func (c *SHA256_CTX) Update(data unsafe.Pointer, n uintptr) c.Int { return 0 }

func (c *SHA256_CTX) UpdateBytes(data []byte) c.Int {
	return c.Update(unsafe.Pointer(unsafe.SliceData(data)), uintptr(len(data)))
}

func (c *SHA256_CTX) UpdateString(data string) c.Int {
	return c.Update(unsafe.Pointer(unsafe.StringData(data)), uintptr(len(data)))
}

// OSSL_DEPRECATEDIN_3_0 int SHA256_Final(unsigned char *md, SHA256_CTX *c);
//
//go:linkname sha256Final C.SHA256_Final
func sha256Final(md *byte, c *SHA256_CTX) c.Int

func (c *SHA256_CTX) Final(md *byte) c.Int {
	return sha256Final(md, c)
}

// OSSL_DEPRECATEDIN_3_0 void SHA256_Transform(SHA256_CTX *c, const unsigned char *data);
//
// llgo:link (*SHA256_CTX).Transform C.SHA256_Transform
func (c *SHA256_CTX) Transform(data *byte) {}

// unsigned char *SHA224(const unsigned char *d, size_t n, unsigned char *md);
//
//go:linkname SHA224 C.SHA224
func SHA224(data unsafe.Pointer, n uintptr, md *byte) *byte

func SHA224Bytes(data []byte, md *byte) *byte {
	return SHA224(unsafe.Pointer(unsafe.SliceData(data)), uintptr(len(data)), md)
}

func SHA224String(data string, md *byte) *byte {
	return SHA224(unsafe.Pointer(unsafe.StringData(data)), uintptr(len(data)), md)
}

// unsigned char *SHA256(const unsigned char *d, size_t n, unsigned char *md);
//
//go:linkname SHA256 C.SHA256
func SHA256(data unsafe.Pointer, n uintptr, md *byte) *byte

func SHA256Bytes(data []byte, md *byte) *byte {
	return SHA256(unsafe.Pointer(unsafe.SliceData(data)), uintptr(len(data)), md)
}

func SHA256String(data string, md *byte) *byte {
	return SHA256(unsafe.Pointer(unsafe.StringData(data)), uintptr(len(data)), md)
}
