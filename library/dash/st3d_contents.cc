/*
Copyright 2016 Google Inc. All rights reserved.

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

#include "library/dash/st3d_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// For more info, see:
// https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.md#stereoscopic-3d-video-box-st3d
// aligned(8) class Stereoscopic3D extends FullBox('st3d', 0, 0) {
//   unsigned int(8) stereo_mode;
// }
size_t St3dContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, 1, length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 1 byte is required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  stereo_mode_ = *ptr;
  ++ptr;
  return ptr - buffer;
}

std::string St3dContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " Stereo Mode: " + PrettyPrintValue(stereo_mode_);
  return result;
}
}  // namespace dash2hls

