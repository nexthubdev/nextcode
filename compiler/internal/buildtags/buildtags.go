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

package buildtags

import (
	"fmt"
	"go/build"
	"io"
	"io/fs"
	"strings"
)

// checkTags checks which build tags are valid by creating virtual test files
// and using build.Context.MatchFile to verify them
func CheckTags(buildFlags []string, testTags map[string]bool) {
	buildCtx := build.Default
	buildCtx.BuildTags = parseBuildTags(buildFlags)

	// Create virtual filesystem
	vfs := &virtualFS{
		files: make(map[string]virtualFile),
	}

	// Generate virtual files for each test tag
	i := 0
	fileToTag := make(map[string]string) // Map to track which file corresponds to which tag
	for tag := range testTags {
		fileName := fmt.Sprintf("a%02d.go", i)
		content := fmt.Sprintf("// +build %s\n\npackage check\n", tag)
		vfs.files[fileName] = virtualFile{
			name:    fileName,
			content: content,
			dir:     ".",
		}
		fileToTag[fileName] = tag
		i++
	}

	// Override OpenFile to return our virtual file contents
	buildCtx.OpenFile = func(name string) (io.ReadCloser, error) {
		if file, ok := vfs.files[name]; ok {
			return io.NopCloser(strings.NewReader(file.content)), nil
		}
		return nil, fs.ErrNotExist
	}

	// Check each file against build context
	for fileName, tag := range fileToTag {
		match, err := buildCtx.MatchFile(".", fileName)
		if err == nil && match {
			testTags[tag] = true
		}
	}
}

// virtualFile represents a virtual build tag check file
type virtualFile struct {
	name    string
	content string
	dir     string
}

// virtualFS implements a virtual filesystem for build tag checking
type virtualFS struct {
	files map[string]virtualFile
}

func parseBuildTags(buildFlags []string) []string {
	buildTags := make([]string, 0)
	// Extract tags from buildFlags
	for i := 0; i < len(buildFlags); i++ {
		flag := buildFlags[i]
		if flag == "-tags" && i+1 < len(buildFlags) {
			// Handle "-tags xxx" format
			tags := strings.FieldsFunc(buildFlags[i+1], func(r rune) bool {
				return r == ',' || r == ' '
			})
			buildTags = append(buildTags, tags...)
			i++ // Skip the next item since we've processed it
		} else if strings.HasPrefix(flag, "-tags=") {
			// Handle "-tags=xxx" format
			value := strings.TrimPrefix(flag, "-tags=")
			tags := strings.FieldsFunc(value, func(r rune) bool {
				return r == ',' || r == ' '
			})
			buildTags = append(buildTags, tags...)
		}
	}

	// Remove duplicates from tags
	seen := make(map[string]bool)
	uniqueBuildTags := make([]string, 0, len(buildTags))
	for _, tag := range buildTags {
		if !seen[tag] {
			seen[tag] = true
			uniqueBuildTags = append(uniqueBuildTags, tag)
		}
	}
	return uniqueBuildTags
}
