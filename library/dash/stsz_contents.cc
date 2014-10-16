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

#include "library/dash/stsz_contents.h"

#include <stdio.h>

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-12 for details.
// aligned(8) class SampleSizeBox extends FullBox(‘stsz’, version = 0, 0) {
//   unsigned int(32) sample_size;
//   unsigned int(32) sample_count;
//   if (sample_size==0) {
//     for (i=1; i <= sample_count; i++) {
//       unsigned int(32) entry_size;
//     }
//   }
// }
size_t StszContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, 2 * sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 8 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  sample_size_ = ntohlFromBuffer(ptr);
  ptr += sizeof(sample_size_);
  sample_count_ = ntohlFromBuffer(ptr);
  ptr += sizeof(sample_count_);
  for (uint32_t count = 0; count < sample_count_; ++count) {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
               "Not enough bytes to parse sample %d of %d\n", count,
               sample_count_);
      DASH_LOG((BoxName() + " too short").c_str(),
               "Could not parse sample_count_",
               (std::string(buffer) +
                DumpMemory(reinterpret_cast<uint8_t*>(buffer),
                           length)).c_str());
      return DashParser::kParseFailure;
    }
    uint32_t sample_size = ntohlFromBuffer(ptr);
    samples_sizes_.push_back(sample_size);
    ptr += sizeof(sample_size);
  }
  return ptr - buffer;
}

std::string StszContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " samples: " + PrettyPrintValue(sample_count_);
  for (std::vector<uint32_t>::const_iterator iter = samples_sizes_.begin();
       iter != samples_sizes_.end(); ++iter) {
    result += "\n" + indent + "  " + PrettyPrintValue(*iter);
  }
  return result;
}
}  // namespace dash2hls
