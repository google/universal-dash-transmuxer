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

#include "library/dash/equi_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// For more info, see:
// https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.md#equirectangular-projection-box-equi
// aligned(8) class EquirectangularProjection ProjectionDataBox('equi', 0, 0) {
//   unsigned int(32) projection_bounds_top;
//   unsigned int(32) projection_bounds_bottom;
//   unsigned int(32) projection_bounds_left;
//   unsigned int(32) projection_bounds_right;
// }
size_t EquiContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, 4 * sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 16 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  // The values are unsigned 0.32 fixed point values.
  projection_bounds_top_ = ntohlFromBuffer(ptr) / 0x1p32;
  ptr += sizeof(uint32_t);
  projection_bounds_bottom_ = ntohlFromBuffer(ptr) / 0x1p32;
  ptr += sizeof(uint32_t);
  projection_bounds_left_ = ntohlFromBuffer(ptr) / 0x1p32;
  ptr += sizeof(uint32_t);
  projection_bounds_right_ = ntohlFromBuffer(ptr) / 0x1p32;
  ptr += sizeof(uint32_t);
  return ptr - buffer;
}

std::string EquiContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " Bounds Top: " + PrettyPrintValue(projection_bounds_top_) +
      " Bounds Bottom: " + PrettyPrintValue(projection_bounds_bottom_) +
      " Bounds Left: " + PrettyPrintValue(projection_bounds_left_) +
      " Bounds Right: " + PrettyPrintValue(projection_bounds_right_);
  return result;
}
}  // namespace dash2hls

