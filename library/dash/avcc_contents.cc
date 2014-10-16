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

#include "library/dash/avcc_contents.h"
#include "library/dash/avc_decoder_configuration_record.h"

#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-12 for details.
//
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
size_t AvcCContents::Parse(const uint8_t *buffer, size_t length) {
  return avc_record_.Parse(buffer, length);
}

std::string AvcCContents::PrettyPrint(std::string indent) const {
  std::string result = BoxContents::PrettyPrint(indent);
  result += avc_record_.PrettyPrint(indent);
  return result;
}
}  // namespace dash2hls
