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

#include "library/dash/prhd_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// For more info, see:
// https://github.com/google/spatial-media/blob/master/docs/spherical-video-v2-rfc.md#projection-header-box-prhd
// aligned(8) class ProjectionHeader extends FullBox('prhd', 0, 0) {
//   int(32) pose_yaw_degrees;
//   int(32) pose_pitch_degrees;
//   int(32) pose_roll_degrees;
// }
size_t PrhdContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, 3 * sizeof(int32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 12 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  // The values are signed 16.16 fixed point values.
  pose_yaw_degrees_ = static_cast<int32_t>(ntohlFromBuffer(ptr)) / 0x1p16;
  ptr += sizeof(int32_t);
  pose_pitch_degrees_ = static_cast<int32_t>(ntohlFromBuffer(ptr)) / 0x1p16;
  ptr += sizeof(int32_t);
  pose_roll_degrees_ = static_cast<int32_t>(ntohlFromBuffer(ptr)) / 0x1p16;
  ptr += sizeof(int32_t);
  return ptr - buffer;
}

std::string PrhdContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " Yaw: " + PrettyPrintValue(pose_yaw_degrees_) +
      " Pitch: " + PrettyPrintValue(pose_pitch_degrees_) +
      " Roll: " + PrettyPrintValue(pose_roll_degrees_);
  return result;
}
}  // namespace dash2hls

