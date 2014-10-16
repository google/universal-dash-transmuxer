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

#include "library/dash/avc_decoder_configuration_record.h"

#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {
namespace internal {
// Populates one parameter set, either a sequence or a picture.  Returns the
// new ptr location.  buffer and length are used to verify no overruns.
const uint8_t* GetParameter(const uint8_t* ptr,
                            const uint8_t* buffer,
                            size_t length,
                            std::vector<std::vector<uint8_t> >* set) {
  uint16_t parameter_length = dash2hls::ntohsFromBuffer(ptr);
  ptr += sizeof(parameter_length);
  if (!EnoughBytesToParse(ptr - buffer, parameter_length, length)) {
    DASH_LOG("AvcC too short", "Failed to get sequence length",
             DumpMemory(buffer, length).c_str());
    return nullptr;
  }
  std::vector<uint8_t> parameter;
  parameter.resize(parameter_length);
  memcpy(&parameter[0], ptr, parameter_length);
  ptr += parameter_length;
  set->push_back(parameter);
  return ptr;
}
}  // namespace internal

// aligned(8) class AVCDecoderConfigurationRecord {
//   unsigned int(8) configurationVersion = 1;
//   unsigned int(8) AVCProfileIndication;
//   unsigned int(8) profile_compatibility;
//   unsigned int(8) AVCLevelIndication;
//   bit(6) reserved = ‘111111’b;
//   unsigned int(2) lengthSizeMinusOne;
//   bit(3) reserved = ‘111’b;
//   unsigned int(5) numOfSequenceParameterSets;
//   for (i=0; i< numOfSequenceParameterSets; i++) {
//     unsigned int(16) sequenceParameterSetLength ;
//     bit(8*sequenceParameterSetLength) sequenceParameterSetNALUnit;
//   }
//   unsigned int(8) numOfPictureParameterSets;
//   for (i=0; i< numOfPictureParameterSets; i++) {
//     unsigned int(16) pictureParameterSetLength;
//     bit(8*pictureParameterSetLength) pictureParameterSetNALUnit;
//   }
// }
size_t AvcDecoderConfigurationRecord::Parse(const uint8_t* buffer,
                                            size_t length) {
  const uint8_t* ptr = buffer;
  if (!EnoughBytesToParse(0, 6, length)) {
    DASH_LOG("AVCDecoderConfigurationRecord too short",
             "At least 6 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  version_ = ptr[0];
  ++ptr;
  avc_profile_indication_ = ptr[0];
  ++ptr;
  profile_compatibility_ = ptr[0];
  ++ptr;
  avc_level_indication_ = ptr[0];
  ++ptr;
  length_size_minus_one_ = ptr[0] & 0x3;
  ++ptr;
  sequence_parameter_sets_count_ = ptr[0] & 0x1f;
  ++ptr;
  for (size_t count = 0; count < sequence_parameter_sets_count_; ++count) {
    ptr = internal::GetParameter(ptr, buffer, length,
                                 &sequence_parameter_sets_);
    if (!ptr) {
      DASH_LOG("AVCDecoderConfigurationRecord too short",
               "Failed to get sps",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
  }
  if (!EnoughBytesToParse(ptr - buffer, 1, length)) {
    DASH_LOG("AVCDecoderConfigurationRecord too short",
             "Failed to get number of pps",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  picture_parameter_sets_count_ = ptr[0];
  ++ptr;
  for (size_t count = 0; count < picture_parameter_sets_count_; ++count) {
    ptr = internal::GetParameter(ptr, buffer, length,
                                 &picture_parameter_sets_);
    if (!ptr) {
      DASH_LOG("AVCDecoderConfigurationRecord too short",
               "Failed to get pps",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
  }
  return ptr - buffer;
}

std::string AvcDecoderConfigurationRecord::PrettyPrint(
    std::string indent) const {
  std::string result;
  result += " Version: " + PrettyPrintValue(version_);
  result += " AVCProfile: " + PrettyPrintValue(avc_profile_indication_);
  result += " ProfileCompat: " + PrettyPrintValue(profile_compatibility_);
  result += " avc_level: " + PrettyPrintValue(avc_level_indication_);
  result += " length_minus_one: " + PrettyPrintValue(length_size_minus_one_);
  for (size_t count = 0; count < sequence_parameter_sets_count_; ++count) {
    result += "\n" + indent + "  sequence NALU: " +
        PrettyPrintValue(sequence_parameter_sets_[count].size()) + " bytes";
  }
  for (size_t count = 0; count < picture_parameter_sets_count_; ++count) {
    result += "\n" + indent + "  picture NALU: " +
        PrettyPrintValue(picture_parameter_sets_[count].size()) + " bytes";
  }
  return result;
}
}  // namespace dash2hls
