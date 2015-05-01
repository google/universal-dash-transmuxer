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

#include "library/dash/elst_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-12 for details.
// aligned(8) class EditListBox extends FullBox(‘elst’, version, 0) {
//   unsigned int(32) entry_count;
//   for (i=1; i <= entry_count; i++) {
//     if (version==1) {
//       unsigned int(64) segment_duration;
//       int(64) media_time;
//     } else { // version==0
//       unsigned int(32) segment_duration;
//       int(32) media_time;
//     }
//     int(16) media_rate_integer;
//     int(16) media_rate_fraction = 0;
//   }
// }
size_t ElstContents::Parse(const uint8_t *buffer, size_t length) {
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
  for (uint32_t count = 0; count < entry_count_; ++count) {
    Entry entry;
    if (version_ == 0) {
      if (!EnoughBytesToParse(ptr - buffer, 2 * sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get segment_duration and media_time",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      entry.segment_duration_ = ntohlFromBuffer(ptr);
      ptr += sizeof(uint32_t);
      entry.media_time_ = ntohlFromBuffer(ptr);
      ptr += sizeof(int32_t);
    } else if (version_ == 1){
      if (!EnoughBytesToParse(ptr - buffer, 2 * sizeof(uint64_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get segment_duration and media_time",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      entry.segment_duration_ = ntohllFromBuffer(ptr);
      ptr += sizeof(entry.segment_duration_);
      entry.media_time_ = ntohllFromBuffer(ptr);
      ptr += sizeof(entry.media_time_);
    } else {
      DASH_LOG((BoxName() + " unsupported version").c_str(),
               "Only parses up to version 1",
               DumpMemory(buffer, length).c_str());
      return length;
    }

    if (!EnoughBytesToParse(ptr - buffer, 2 * sizeof(int16_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Can not get media_rate_integer and media_rate_fraction",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    entry.media_rate_integer_ = ntohsFromBuffer(ptr);
    ptr += sizeof(entry.media_rate_integer_);
    entry.media_rate_fraction_ = ntohsFromBuffer(ptr);

    entries_.push_back(entry);
  }
  return ptr - buffer;
}

std::string ElstContents::PrettyPrintEntry(const Entry& entry) const {
  std::string result = "";
  result += "SegmentDuration: " + PrettyPrintValue(entry.segment_duration_);
  result += " MediaTime: " + PrettyPrintValue(entry.media_time_);
  result += " MediaRateInteger: " + PrettyPrintValue(entry.media_rate_integer_);
  result += " MediaRateFraction: " +
      PrettyPrintValue(entry.media_rate_fraction_);
  return result;
}

std::string ElstContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " Entries: " + PrettyPrintValue(entry_count_);
  for (const Entry& entry : entries_) {
    result += "\n" + indent + PrettyPrintEntry(entry);
  }
  return result;
}

}  // namespace dash2hls
