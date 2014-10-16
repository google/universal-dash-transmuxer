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

#include "library/dash/audio_sample_entry_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace {
const size_t kMinBoxSize = 7 * sizeof(uint32_t);
}  // namespace

namespace dash2hls {

// See ISO 14496-15 and 14495-12 for details.
//
// class AudioSampleEntry(codingname) extends SampleEntry (codingname){
//   const unsigned int(32)[2] reserved = 0;
//   template unsigned int(16) channelcount = 2;
//   template unsigned int(16) samplesize = 16;
//   unsigned int(16) pre_defined = 0;
//   const unsigned int(16) reserved = 0 ;
//   template unsigned int(32) samplerate = { default samplerate of media}<<16;
// }
size_t AudioSampleEntryContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* volatile ptr =
      buffer + SampleEntryContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, kMinBoxSize, length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 28 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  // Reserved bytes to skip
  ptr += sizeof(uint32_t) * 2;
  channel_count_ = ntohsFromBuffer(ptr);
  ptr += sizeof(channel_count_);
  sample_size_ = ntohsFromBuffer(ptr);
  ptr += sizeof(sample_size_);
  // Skip pre_defined and reserved.
  ptr += sizeof(uint16_t) * 2;
  sample_rate_ = ntohlFromBuffer(ptr);
  ptr += sizeof(sample_rate_);

  size_t bytes_left = length - (ptr - buffer);
  if (bytes_left > 0) {
    dash_parser_ = new DashParser;
    dash_parser_->set_current_position(stream_position_ + bytes_left);
    size_t bytes_parsed = dash_parser_->Parse(ptr, bytes_left);
    if (bytes_parsed != bytes_left) {
      return DashParser::kParseFailure;
    }
    ptr += bytes_parsed;
  }

  return ptr - buffer;
}

std::string AudioSampleEntryContents::PrettyPrint(std::string indent) const {
  std::string result = " Channels: " + PrettyPrintValue(channel_count_);
  result += " Sample Size: " + PrettyPrintValue(sample_size_);
  result += " Sample Rate: " + PrettyPrintValue(sample_rate_);
  result += SampleEntryContents::PrettyPrint(indent);
  return result;
}
}  // namespace dash2hls
