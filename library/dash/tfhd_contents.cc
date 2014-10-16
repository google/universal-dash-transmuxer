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

#include "library/dash/tfhd_contents.h"

#include <stdio.h>

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// Currently only parses version 1 of a sidx box.
//
// See ISO 14496-12 for details.
// aligned(8) class TrackFragmentHeaderBox
// extends FullBox(‘tfhd’, 0, tf_flags){
//   unsigned int(32) track_ID;
//   // all the following are optional fields
//   unsigned int(64) base_data_offset;
//   unsigned int(32) sample_description_index;
//   unsigned int(32) default_sample_duration;
//   unsigned int(32) default_sample_size;
//   unsigned int(32) default_sample_flags
// }
size_t TfhdContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (version_ == 0) {
    track_id_ = ntohlFromBuffer(ptr);
    ptr += sizeof(track_id_);
    if (flags_ & kBaseDataOffsetPresentMask) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint64_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get base_data_offset_",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      base_data_offset_ = ntohllFromBuffer(ptr);
      ptr += sizeof(base_data_offset_);
    }
    if (flags_ & kSampleDecryptionIndexPresentMask) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get sample_description_index_",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      sample_description_index_ = ntohlFromBuffer(ptr);
      ptr += sizeof(sample_description_index_);
    }
    if (flags_ & kDefaultSampleDurationPresentMask) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get default_sample_duration",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      default_sample_duration_ = ntohlFromBuffer(ptr);
      ptr += sizeof(default_sample_duration_);
    }
    if (flags_ & kDefaultSampleSizePresentMask) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get default_sample_size_",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      default_sample_size_ = ntohlFromBuffer(ptr);
      ptr += sizeof(default_sample_size_);
    }
    if (flags_ & kDefaultSampleFlagsPresentMask) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get default_sample_flags_",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      default_sample_flags_ = ntohlFromBuffer(ptr);
      ptr += sizeof(default_sample_flags_);
    }
  } else {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "tkhd version %d not supported\n", version_);
    DASH_LOG("Unsupported version", buffer, nullptr);
  }
  return ptr - buffer;
}

std::string TfhdContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " Track ID:" + PrettyPrintValue(track_id_);
  if (flags_ & kBaseDataOffsetPresentMask) {
    result += " Base Data Offset:" + PrettyPrintValue(base_data_offset_);
  }
  if (flags_ & kSampleDecryptionIndexPresentMask) {
    result += " Sample Description Index:" +
        PrettyPrintValue(sample_description_index_);
  }
  if (flags_ & kDefaultSampleDurationPresentMask) {
    result += " Default Duration:" +
        PrettyPrintValue(default_sample_duration_);
  }
  if (flags_ & kDefaultSampleSizePresentMask) {
    result += " Default Size:" + PrettyPrintValue(default_sample_size_);
  }
  if (flags_ & kDefaultSampleFlagsPresentMask) {
    result += " Default Flags:" + PrettyPrintValue(default_sample_flags_);
  }
  return result;
}
}  // namespace dash2hls
