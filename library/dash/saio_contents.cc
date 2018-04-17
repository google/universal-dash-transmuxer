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

#include "library/dash/saio_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

bool SaioContents::IsAuxInfoPresent() const {
  return flags_ & kAuxInfoPresentMask;
}

// Currently only parses version 1 of a sidx box.
//
// See ISO 14496-12 for details.
// aligned(8) class SampleAuxiliaryInformationOffsetsBox
// extends FullBox(‘saio’, version, flags)
// {
//   if (flags & 1) {
//     unsigned int(32) aux_info_type;
//     unsigned int(32) aux_info_type_parameter;
//   }
//   unsigned int(32) entry_count;
//   if ( version == 0 ) {
//     unsigned int(32) offset[ entry_count ];
//   }
//   else {
//     unsigned int(64) offset[ entry_count ];
//   }
size_t SaioContents::Parse(const uint8_t *buffer, size_t length) {
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
  if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 1 more bytes is required for sample count",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  offsets_.resize(ntohlFromBuffer(ptr));
  ptr += sizeof(uint32_t);

  if (version_ == 0) {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t) * offsets_.size(),
                            length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Not enough data for samples",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    for (size_t count = 0; count < offsets_.size(); ++count) {
      offsets_[count] = ntohlFromBuffer(ptr);
      ptr += sizeof(uint32_t);
    }
  } else {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint64_t) * offsets_.size(),
                            length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Not enough data for samples",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    for (size_t count = 0; count < offsets_.size(); ++count) {
      offsets_[count] = ntohllFromBuffer(ptr);
      ptr += sizeof(uint64_t);
    }
  }
  return ptr - buffer;
}

std::string SaioContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  if (IsAuxInfoPresent()) {
    result += " Type:" + PrettyPrintValue(aux_info_type_);
    result += " Parameter:" + PrettyPrintValue(aux_info_type_parameter_);
  }
  result += " Offsets:" + PrettyPrintValue(offsets_.size());
  if (g_verbose_pretty_print) {
    for (uint32_t count = 0; count < offsets_.size(); ++count) {
      result += "\n" + indent + "  " + PrettyPrintValue(offsets_[count]);
    }
  }
  return result;
}
}  //  namespace dash2hls
