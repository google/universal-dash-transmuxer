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

#include "library/dash/stsd_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-12 for details.
// aligned(8) class SampleDescriptionBox (unsigned int(32) handler_type)
// extends FullBox('stsd', 0, 0){
//   int i ;
//   unsigned int(32) entry_count;
//   for (i = 1 ; i <= entry_count ; i++){
//     switch (handler_type){
//       case ‘soun’: // for audio tracks
//         AudioSampleEntry();
//         break;
//       case ‘vide’: // for video tracks
//         VisualSampleEntry();
//         break;
//       case ‘hint’: // Hint track
//         HintSampleEntry();
//         break;
//       case ‘meta’: // Metadata track
//         MetadataSampleEntry();
//         break;
//     }
//   }
// }
size_t StsdContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  entry_count_ = ntohlFromBuffer(ptr);
  ptr += sizeof(entry_count_);
  size_t bytes_parsed = BoxContents::Parse(ptr, length - (ptr - buffer));
  if (bytes_parsed == 0) {
    return DashParser::kParseFailure;
  }
  ptr += bytes_parsed;
  return ptr - buffer;
}

std::string StsdContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " descriptions: " + PrettyPrintValue(entry_count_);
  result += BoxContents::PrettyPrint(indent);
  return result;
}
}  // namespace dash2hls
