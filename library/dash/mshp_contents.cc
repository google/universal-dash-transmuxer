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

#include "library/dash/mshp_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// For more info, see:
// go/pir-design
//  aligned(8) class MeshProjection extends FullBox('mshp', 0, 0) {
//   unsigned int(32) crc32;
//   unsigned int(32) encoding_four_cc;
//   ...Other boxes, possibly compressed...
// }
size_t MshpContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, 2 * sizeof(uint32_t) + 1, length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 1 byte is required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  crc32_ = ntohlFromBuffer(ptr);
  ptr += sizeof(crc32_);
  encoding_four_cc_ = ntohlFromBuffer(ptr);
  ptr += sizeof(encoding_four_cc_);
  raw_data_ = ptr;
  raw_data_length_ = length - (ptr - buffer);
  return ptr - buffer;
}

std::string MshpContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " CRC-32: " + PrettyPrintValue(crc32_) +
      " Encoding: " + PrettyPrintValue(encoding_four_cc_) +
      " " + PrettyPrintValue(raw_data_length_) + " bytes";
  return result;
}
}  // namespace dash2hls

