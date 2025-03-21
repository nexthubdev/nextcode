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

package main

import (
	"bytes"
	"crypto/sha1"
	"encoding/base64"
	"go/build"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"time"

	"github.com/goplus/gop/tool"
	"github.com/goplus/gop/x/gocmd"
	"github.com/goplus/gop/x/gopenv"
)

func fileIsDirty(srcMod time.Time, destFile string) bool {
	fiDest, err := os.Stat(destFile)
	if err != nil {
		return true
	}
	return srcMod.After(fiDest.ModTime())
}

func runGoFile(dir, file, fname string) {
	gopEnv := gopenv.Get()
	conf := &tool.Config{Gop: gopEnv}
	confCmd := &gocmd.BuildConfig{Gop: gopEnv}
	fi, err := os.Stat(file)
	if err != nil {
		log.Panicln(err)
	}
	absFile, _ := filepath.Abs(file)
	hash := sha1.Sum([]byte(absFile))
	outFile := dir + "g" + base64.RawURLEncoding.EncodeToString(hash[:]) + fname
	if fileIsDirty(fi.ModTime(), outFile) {
		err = tool.RunFiles(outFile, []string{file}, nil, conf, confCmd)
		if err != nil {
			os.Remove(outFile)
			switch e := err.(type) {
			case *exec.ExitError:
				os.Exit(e.ExitCode())
			default:
				log.Fatalln("runGoFile:", err)
			}
		}
	}
}

var (
	goRunPrefix = []byte("// run\n")
)

var (
	skipFileNames = map[string]struct{}{
		"convert4.go":       {},
		"peano.go":          {},
		"bug295.go":         {}, // import . "XXX"
		"issue15071.dir":    {}, // dir
		"issue29612.dir":    {},
		"issue31959.dir":    {},
		"issue29504.go":     {}, // line
		"issue18149.go":     {},
		"issue22662.go":     {},
		"issue27201.go":     {},
		"issue46903.go":     {},
		"issue50190.go":     {}, // interesting, should be fixed
		"nilptr_aix.go":     {},
		"inline_literal.go": {},
		"returntype.go":     {}, // not a problem
		"unsafebuiltins.go": {},
	}
)

func gopTestRunGo(dir string) {
	home, _ := os.UserHomeDir()
	targetDir := home + "/.gop/run/"
	os.MkdirAll(targetDir, 0777)
	filepath.Walk(dir, func(file string, fi os.FileInfo, err error) error {
		name := fi.Name()
		if err != nil || fi.IsDir() {
			if _, ok := skipFileNames[name]; ok {
				return filepath.SkipDir
			}
			return nil
		}
		if _, ok := skipFileNames[name]; ok {
			return nil
		}
		ext := filepath.Ext(name)
		if ext != ".go" {
			return nil
		}
		data, err := ioutil.ReadFile(file)
		if err != nil {
			log.Panicln(err)
		}
		if !bytes.HasPrefix(data, goRunPrefix) {
			return nil
		}
		log.Println("==> gop run -v", file)
		runGoFile(targetDir, file, name)
		return nil
	})
}

// goptestgo: run all $GOROOT/test/*.go
func main() {
	dir := filepath.Join(build.Default.GOROOT, "test")
	gopTestRunGo(dir)
}
