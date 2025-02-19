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

package yap

import (
	"encoding/json"
	"io"
	"log"
	"net/http"
	"strconv"
	"strings"
)

type Context struct {
	*http.Request
	http.ResponseWriter

	engine *Engine
}

func (p *Context) setParam(name, val string) {
	p.ParseForm()
	p.Form.Set(name, val)
}

// Gop_Env returns the value associated with the name.
// If the name exists in URL query, it returns the first value for the name.
func (p *Context) Gop_Env(name string) string {
	return p.Param(name)
}

// Param returns the value associated with the name.
// If the name exists in URL query, it returns the first value for the name.
func (p *Context) Param(name string) string {
	return p.FormValue(name)
}

func (p *Context) ParamInt(name string, defval int) int {
	ret := p.Param(name)
	if ret != "" {
		if v, err := strconv.Atoi(ret); err == nil {
			return v
		}
	}
	return defval
}

// Accept header specifies:
// Accept: <MIME_type>/<MIME_subtype>
// Accept: <MIME_type>/*
// Accept: */*
// Multiple types, weighted with the quality value syntax:
// Accept: text/html, application/xhtml+xml, application/xml;q=0.9, image/webp, */*;q=0.8
// FIXME: 1. quality value not supported, 2. don't need parse all, just find the first match with a spliter iterator
func (p *Context) Accept(mime ...string) string {
	accept := p.Request.Header.Get("Accept")
	for _, m := range mime {
		if acceptMime(accept, m) {
			return m
		}
	}
	return ""
}

func acceptMime(accept string, mime string) bool {
	for accept != "" {
		item, left := acceptNext(accept)
		if item == mime || (strings.HasPrefix(item, mime) && item[len(mime)] == ';') {
			return true
		}
		accept = left
	}
	return false
}

func acceptNext(accept string) (item, left string) {
	item = strings.TrimLeft(accept, " ")
	if before, after, found := strings.Cut(item, ","); found {
		return before, after
	}
	left = ""
	return
}

// Redirect replies to the request with a redirect to url,
// which may be a path relative to the request path.
func (p *Context) Redirect(url string, code ...int) {
	statusCode := http.StatusFound
	if code != nil {
		statusCode = code[0]
	}
	http.Redirect(p.ResponseWriter, p.Request, url, statusCode)
}

func (p *Context) TEXT(code int, mime string, text string) {
	w := p.ResponseWriter
	h := w.Header()
	h.Set("Content-Length", strconv.Itoa(len(text)))
	h.Set("Content-Type", mime)
	w.WriteHeader(code)
	io.WriteString(w, text)
}

func (p *Context) DATA(code int, mime string, data []byte) {
	w := p.ResponseWriter
	h := w.Header()
	h.Set("Content-Length", strconv.Itoa(len(data)))
	h.Set("Content-Type", mime)
	w.WriteHeader(code)
	w.Write(data)
}

func (p *Context) PrettyJSON(code int, data interface{}) {
	msg, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		panic(err)
	}
	p.DATA(code, "application/json", msg)
}

func (p *Context) JSON(code int, data interface{}) {
	msg, err := json.Marshal(data)
	if err != nil {
		panic(err)
	}
	p.DATA(code, "application/json", msg)
}

func (p *Context) YAP(code int, yapFile string, data interface{}) {
	w := p.ResponseWriter
	t := p.engine.templ(yapFile)
	if t == nil {
		log.Panicln("YAP: not find template:", yapFile)
	}
	h := w.Header()
	h.Set("Content-Type", "text/html")
	err := t.Execute(w, data)
	if err != nil {
		log.Panicln("YAP:", err)
	}
}
