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

#include "library/dash/mvhd_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-12 for details.
// aligned(8) class MovieHeaderBox extends FullBox(‘mvhd’, version, 0) {
//   if (version==1) {
//     unsigned int(64) creation_time;
//     unsigned int(64) modification_time;
//     unsigned int(32) timescale;
//     unsigned int(64) duration;
//   } else { // version==0
//     unsigned int(32) creation_time;
//     unsigned int(32) modification_time;
//     unsigned int(32) timescale;
//     unsigned int(32) duration;
//   }
//   template int(32) rate = 0x00010000; // typically 1.0
//   template int(16) volume = 0x0100; // typically, full volume
//   const bit(16) reserved = 0;
//   const unsigned int(32)[2] reserved = 0;
//   template int(32)[9] matrix =
//       { 0x00010000,0,0,0,0x00010000,0,0,0,0x40000000 };
//   // Unity matrix
//   bit(32)[6] pre_defined = 0;
//   unsigned int(32) next_track_ID;
// }

size_t MvhdContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if (ptr == buffer) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "Header not found.",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (version_ == kVersion0) {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t) * 4, length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Mandatory fields not found.",
               DumpMemory(buffer, length).c_str());
    }
    creation_time_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
    modification_time_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
    timescale_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
    duration_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
  } else {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint64_t) * 4, length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Mandatory fields not found.",
               DumpMemory(buffer, length).c_str());
    }
    creation_time_ = ntohllFromBuffer(ptr);
    ptr += sizeof(uint64_t);
    modification_time_ = ntohllFromBuffer(ptr);
    ptr += sizeof(uint64_t);
    timescale_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
    duration_ = ntohllFromBuffer(ptr);
    ptr += sizeof(uint64_t);
  }
  return ptr - buffer;
}

std::string MvhdContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " Creation Time: " + PrettyPrintValue(creation_time_);
  result += " Modification Time: " + PrettyPrintValue(modification_time_);
  result += " Timescale: " + PrettyPrintValue(timescale_);
  result += " Duration: " + PrettyPrintValue(duration_);
  return result;
}
}  //  namespace dash2hls
