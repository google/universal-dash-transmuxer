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

#include "library/dash/trex_contents.h"

#include <stdio.h>

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-12 for details.
// aligned(8) class TrackExtendsBox extends FullBox(‘trex’, 0, 0){
//   unsigned int(32) track_ID;
//   unsigned int(32) default_sample_description_index;
//   unsigned int(32) default_sample_duration;
//   unsigned int(32) default_sample_size;
//   unsigned int(32) default_sample_flags
// }
size_t TrexContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, 5 * sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 20 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  track_id_ = ntohlFromBuffer(ptr);
  ptr += sizeof(track_id_);
  default_sample_description_index_ = ntohlFromBuffer(ptr);
  ptr += sizeof(default_sample_description_index_);
  default_sample_duration_ = ntohlFromBuffer(ptr);
  ptr += sizeof(default_sample_duration_);
  default_sample_size_ = ntohlFromBuffer(ptr);
  ptr += sizeof(default_sample_size_);
  default_sample_flags_ = ntohlFromBuffer(ptr);
  ptr += sizeof(default_sample_flags_);
  return ptr - buffer;
}

std::string TrexContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " Track ID:" + PrettyPrintValue(track_id_);
  result += " Default Sample Description Index:" +
      PrettyPrintValue(default_sample_description_index_);
  result += " Default Duration:" +
      PrettyPrintValue(default_sample_duration_);
  result += " Default Size:" + PrettyPrintValue(default_sample_size_);
  result += " Default Flags:" + PrettyPrintValue(default_sample_flags_);
  return result;
}
}  // namespace dash2hls
