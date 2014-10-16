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

#include "library/dash/saiz_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

bool SaizContents::IsAuxInfoPresent() const {
  return flags_ & kAuxInfoPresentMask;
}

// See ISO 14496-12 for details.
// aligned(8) class SampleAuxiliaryInformationSizesBox
// extends FullBox(‘saiz’, version = 0, flags)
// {
//   if (flags & 1) {
//     unsigned int(32) aux_info_type;
//     unsigned int(32) aux_info_type_parameter;
//   }
//   unsigned int(8) default_sample_info_size;
//   unsigned int(32) sample_count;
//   if (default_sample_info_size == 0) {
//     unsigned int(8) sample_info_size[ sample_count ];
//   }
size_t SaizContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (IsAuxInfoPresent()) {
    if (!EnoughBytesToParse(ptr - buffer, 2 * sizeof(uint32_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "At least 8 more bytes are required for aux info",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    aux_info_type_ = ntohlFromBuffer(ptr);
    ptr += sizeof(aux_info_type_);
    aux_info_type_parameter_ = ntohlFromBuffer(ptr);
    ptr += sizeof(aux_info_type_parameter_);
  }
  if (!EnoughBytesToParse(ptr - buffer, sizeof(uint8_t) + sizeof(uint32_t),
                          length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 5 more bytes is required for default size and "
             "sample count",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  default_sample_info_size_ = *ptr;
  ++ptr;
  sizes_.resize(ntohlFromBuffer(ptr));
  ptr += sizeof(uint32_t);
  // saiz boxes can either contain records or everything uses the default size.
  if (default_sample_info_size_ == 0) {
    if (!EnoughBytesToParse(ptr - buffer, sizes_.size(), length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Not enough data for samples",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    memcpy(&sizes_[0], ptr, sizes_.size());
    ptr += sizes_.size();
  } else {
    for (size_t count = 0; count < sizes_.size(); ++count) {
      sizes_[count] = default_sample_info_size_;
    }
  }
  return ptr - buffer;
}

std::string SaizContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  if (IsAuxInfoPresent()) {
    result += " Type:" + PrettyPrintValue(aux_info_type_);
    result += " Parameter:" + PrettyPrintValue(aux_info_type_parameter_);
  }
  result += " Default Size:" + PrettyPrintValue(default_sample_info_size_);
  result += " Sizes:" + PrettyPrintValue(sizes_.size());
  if (g_verbose_pretty_print) {
    for (uint32_t count = 0; count < sizes_.size(); ++count) {
      result += "\n" + indent + "  " + PrettyPrintValue(sizes_[count]);
    }
  }
  return result;
}
}  //  namespace dash2hls
