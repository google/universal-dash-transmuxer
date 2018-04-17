/*
Copyright 2018 Google Inc. All rights reserved.

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

#include "library/dash/sbgp_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// aligned(8) class SampleToGroupBox
// extends FullBox(‘sbgp’, version, 0) {
//   unsigned int(32) grouping_type;
//   if (version == 1) {
//     unsigned int(32) grouping_type_parameter;
//   }
//   unsigned int(32) entry_count;
//   for (i=1; i <= entry_count; i++) {
//     unsigned int(32) sample_count;
//     unsigned int(32) group_description_index;
//   }
// }

size_t SbgpContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  grouping_type_ = ntohlFromBuffer(ptr);
  ptr += sizeof(uint32_t);
  if (version_ == 1) {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Can not get grouping_type_parameter",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    grouping_type_parameter_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
  }
  if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(), "Can not get entry_count",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  entry_count_ = ntohlFromBuffer(ptr);
  ptr += sizeof(uint32_t);
  if (!EnoughBytesToParse(ptr - buffer, 2 * sizeof(uint32_t) * entry_count_,
                          length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "Can not get sample_count and group index",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  for (uint32_t count = 0; count < entry_count_; ++count) {
    GroupEntry entry;
    entry.sample_count_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
    entry.group_description_index_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
    entries_.push_back(entry);
  }
  return ptr - buffer;
}

std::string SbgpContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  return result;
}
}  //  namespace dash2hls
