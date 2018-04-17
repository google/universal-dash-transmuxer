/*
Copyright 2014 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Test FILE objects.  On the Mac there is special code to find the resource.
// Not in a namespace because it might be included by ObjC code.

#ifndef _DASH2HLS_MAC_TEST_FILES_H_
#define _DASH2HLS_MAC_TEST_FILES_H_

#include <stdio.h>

// TODO(seawardt): Merge ObjC tests and vectors into C++ tests where possible

// TODO(seawardt): The vectors these reference are not in the repo!
FILE* Dash2HLS_GetTestVideoFile();
FILE* Dash2HLS_GetTestAudioFile();

FILE* Dash2HLS_GetTestCencVideoFile();
FILE* Dash2HLS_GetTestCencAudioFile();

// TODO(seawardt): Neither of these is called.
// TODO(seawardt): Should we drop these methods and the vectors they access?
FILE* Dash2HLS_GetTestCencVideoHeader();
FILE* Dash2HLS_GetTestCencVideoSegment();

FILE* Dash2HLS_GetMp4BoxInitSegment();

#endif  // _DASH2HLS_MAC_TEST_FILES_H_
