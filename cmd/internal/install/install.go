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

// Package install implements the “gop install” command.
package install

import (
	"fmt"
	"log"
	"os"
	"reflect"

	"github.com/goplus/gogen"
	"github.com/goplus/gop/cl"
	"github.com/goplus/gop/cmd/internal/base"
	"github.com/goplus/gop/tool"
	"github.com/goplus/gop/x/gocmd"
	"github.com/goplus/gop/x/gopprojs"
	"github.com/goplus/mod/modfetch"
)

// gop install
var Cmd = &base.Command{
	UsageLine: "gop install [-debug] [packages]",
	Short:     "Build Go+ files and install target to GOBIN",
}

var (
	flag      = &Cmd.Flag
	flagDebug = flag.Bool("debug", false, "print debug information")
)

func init() {
	Cmd.Run = runCmd
}

func runCmd(cmd *base.Command, args []string) {
	pass := base.PassBuildFlags(cmd)
	err := flag.Parse(args)
	if err != nil {
		log.Fatalln("parse input arguments failed:", err)
	}

	pattern := flag.Args()
	if len(pattern) == 0 {
		pattern = []string{"."}
	}

	projs, err := gopprojs.ParseAll(pattern...)
	if err != nil {
		log.Panicln("gopprojs.ParseAll:", err)
	}

	if *flagDebug {
		modfetch.SetDebug(modfetch.DbgFlagAll)
		gogen.SetDebug(gogen.DbgFlagAll &^ gogen.DbgFlagComments)
		cl.SetDebug(cl.DbgFlagAll)
		cl.SetDisableRecover(true)
	}

	conf, err := tool.NewDefaultConf(".", tool.ConfFlagNoTestFiles, pass.Tags())
	if err != nil {
		log.Panicln("tool.NewDefaultConf:", err)
	}
	defer conf.UpdateCache()

	confCmd := conf.NewGoCmdConf()
	confCmd.Flags = pass.Args
	for _, proj := range projs {
		install(proj, conf, confCmd)
	}
}

func install(proj gopprojs.Proj, conf *tool.Config, install *gocmd.InstallConfig) {
	const flags = tool.GenFlagPrompt
	var obj string
	var err error
	switch v := proj.(type) {
	case *gopprojs.DirProj:
		obj = v.Dir
		err = tool.InstallDir(obj, conf, install, flags)
	case *gopprojs.PkgPathProj:
		obj = v.Path
		err = tool.InstallPkgPath("", v.Path, conf, install, flags)
	case *gopprojs.FilesProj:
		err = tool.InstallFiles(v.Files, conf, install)
	default:
		log.Panicln("`gop install` doesn't support", reflect.TypeOf(v))
	}
	if tool.NotFound(err) {
		fmt.Fprintf(os.Stderr, "gop install %v: not found\n", obj)
	} else if err != nil {
		fmt.Fprintln(os.Stderr, err)
	} else {
		return
	}
	os.Exit(1)
}

// -----------------------------------------------------------------------------
