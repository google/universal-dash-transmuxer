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

#include "library/dash/tfdt_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// Currently only parses version 1 of a sidx box.
//
// See ISO 14496-12 for details.
// aligned(8) class TrackFragmentBaseMediaDecodeTimeBox
// extends FullBox(‘tfdt’, version, 0) {
//   if (version==1) {
//     unsigned int(64) baseMediaDecodeTime;
//   } else { // version==0
//     unsigned int(32) baseMediaDecodeTime;
//   }
// }
size_t TfdtContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (version_ == kVersion0) {
    base_media_decode_time_ = ntohlFromBuffer(buffer + 4);
  } else {
    base_media_decode_time_ = ntohllFromBuffer(buffer + 4);
  }
  return ptr - buffer;
}

std::string TfdtContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " Base Media Decode Time: " +
      PrettyPrintValue(base_media_decode_time_);
  return result;
}
}  //  namespace dash2hls
