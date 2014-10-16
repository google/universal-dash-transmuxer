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

#include "library/dash/trun_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

bool TrunContents::IsBaseDataOffsetPresent() const {
  return flags_ & kBaseDataOffsetPresentMask;
}

bool TrunContents::IsFirstSampleFlagsPresent() const {
  return flags_ & kFirstSampleFlagsPresentMask;
}

bool TrunContents::IsSampleDurationPresent() const {
  return flags_ & kSampleDurationPresentMask;
}

bool TrunContents::IsSampleSizePresent() const {
  return flags_ & kSampleSizePresentMask;
}

bool TrunContents::IsSampleFlagsPresent() const {
  return flags_ & kSampleFlagsPresentMask;
}

bool TrunContents::IsSampleCompositionPresent() const {
  return flags_ & kSampleCompositionPresentMask;
}

// Currently only parses version 1 of a sidx box.
//
// See ISO 14496-12 for details.
// aligned(8) class TrackRunBox
// extends FullBox(‘trun’, version, tr_flags) {
//   unsigned int(32) sample_count;
//   // the following are optional fields
//   signed int(32) data_offset;
//   unsigned int(32) first_sample_flags;
//   // all fields in the following array are optional
//   {
//     unsigned int(32) sample_duration;
//     unsigned int(32) sample_size;
//     unsigned int(32) sample_flags
//     if (version == 0) {
//       unsigned int(32) sample_composition_time_offset;
//     } else {
//       signed int(32) sample_composition_time_offset;
//     }
//   }[ sample_count ]
// }
size_t TrunContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  sample_count_ = ntohlFromBuffer(ptr);
  ptr += sizeof(sample_count_);
  if (IsBaseDataOffsetPresent()) {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Can not get data_offset_",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    data_offset_ = ntohlFromBuffer(ptr);
    ptr += sizeof(data_offset_);
  }
  if (IsFirstSampleFlagsPresent()) {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Can not get first_sample_flags_",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    first_sample_flags_ = ntohlFromBuffer(ptr);
    ptr += sizeof(first_sample_flags_);
  }

  for (uint32_t count = 0; count < sample_count_; ++count) {
    TrackRun run;
    if (IsSampleDurationPresent()) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get run.sample_duration_",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      run.sample_duration_ = ntohlFromBuffer(ptr);
      ptr += sizeof(run.sample_duration_);
    }
    if (IsSampleSizePresent()) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get run.sample_size_",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      run.sample_size_ = ntohlFromBuffer(ptr);
      ptr += sizeof(run.sample_size_);
    }
    if (IsSampleFlagsPresent()) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get run.sample_flags_",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      run.sample_flags_ = ntohlFromBuffer(ptr);
      ptr += sizeof(run.sample_flags_);
    }
    if (IsSampleCompositionPresent()) {
      if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
        DASH_LOG((BoxName() + " too short").c_str(),
                 "Can not get run.sample_composition_time_offset_",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      run.sample_composition_time_offset_ = ntohlFromBuffer(ptr);
      ptr += sizeof(run.sample_composition_time_offset_);
    }
    track_runs_.push_back(run);
  }
  return ptr - buffer;
}

std::string TrunContents::PrettyPrintTrackRun(const TrackRun& run) const {
  std::string result;
  if (IsSampleDurationPresent()) {
    result += " Duration:" + PrettyPrintValue(run.sample_duration_);
  }
  if (IsSampleSizePresent()) {
    result += " Size:" + PrettyPrintValue(run.sample_size_);
  }
  if (IsSampleFlagsPresent()) {
    result += " Flags:" + PrettyPrintValue(run.sample_flags_);
  }
  if (IsSampleCompositionPresent()) {
    result += " Composition Time:" + PrettyPrintValue(
        run.sample_composition_time_offset_);
  }
  return result;
}

std::string TrunContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  if (IsBaseDataOffsetPresent()) {
    result += " Data Offset:" + PrettyPrintValue(data_offset_);
  }
  if (IsFirstSampleFlagsPresent()) {
    result += " First Sample Flags:" + PrettyPrintValue(first_sample_flags_);
  }
  result += " Samples:" + PrettyPrintValue(sample_count_);
  if (g_verbose_pretty_print) {
    for (uint32_t count = 0; count < sample_count_; ++count) {
      result += "\n" + indent + "  " + PrettyPrintTrackRun(track_runs_[count]);
    }
  }
  return result;
}
}  //  namespace dash2hls
