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

#include "library/dash/decoder_descriptor.h"

#include "library/bit_reader.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace {
const size_t kDecoderDescriptorMinSize = 14;
const size_t kStreamTypeOffset = 2;
const size_t kUpStreamOffset = 1;
const uint8_t kUpStreamMask = 0x1;
const uint8_t kAudioObjectTypeSizeInBits = 5;
const uint8_t kSampleFrequencyIndexSizeInBits = 4;
const uint8_t kChannelsSizeInBits = 4;
const uint8_t kExtensionSampleFrequencyIndexSizeInBits = 4;
const uint8_t kExtendedAudioType = 5;
}  // namespace

namespace dash2hls {

// See ISO 14496-3 1.6.2.1 for details.
//
size_t DecoderDescriptor::DecoderSpecificInfo::Parse(const uint8_t* buffer,
                                                     size_t length) {
  size_t bytes_parsed = BaseDescriptor::ParseHeader(buffer, length);
  if (bytes_parsed == 0) {
    return DashParser::kParseFailure;
  }
  const uint8_t* bptr = buffer + bytes_parsed;

  if (get_tag() != kDecSpecificInfoTag) {
    DASH_LOG("Bad DecoderSpecificInfo",
             "Expected kDecSpecificInfoTag.",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }

  BitReader bit_reader(bptr, get_size());
  sbr_present_ = false;
  if (!bit_reader.Read(kAudioObjectTypeSizeInBits, &audio_object_type_)) {
    DASH_LOG("Could not read audio object", "Not enough bits",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (!bit_reader.Read(kSampleFrequencyIndexSizeInBits,
                       &sampling_frequency_index_)) {
    DASH_LOG("Could not read sampling frequency", "Not enough bits",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (sampling_frequency_index_ == 0x0f) {
    DASH_LOG("Unsupported DASH", "No support for samplingFrequencyIndex of 0xf",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (!bit_reader.Read(kChannelsSizeInBits, &channel_config_)) {
    DASH_LOG("Could not read channels", "Not enough bits",
             DumpMemory(buffer, length).c_str());
  }

  if (audio_object_type_ == kExtendedAudioType) {
    extension_audio_object_type_ = audio_object_type_;
    sbr_present_ = true;
    if (!bit_reader.Read(kExtensionSampleFrequencyIndexSizeInBits,
                         &extension_sampling_frequency_index_)) {
      DASH_LOG("Could not read extension sampling frequency index",
               "Not enough bits",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    if (extension_sampling_frequency_index_ == 0x0f) {
      DASH_LOG("Unsupported DASH",
               "No support for extensionSamplingFrequencyIndex of 0xf",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    if (!bit_reader.Read(kAudioObjectTypeSizeInBits, &audio_object_type_)) {
      DASH_LOG("Could not read extension audio object", "Not enough bits",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
  }
  audio_config_.resize(get_size());
  memcpy(&audio_config_[0], bptr, get_size());

  bptr += get_size();
  return bptr - buffer;
}

std::string
DecoderDescriptor::DecoderSpecificInfo::PrettyPrint(std::string indent) const {
  std::string result = BaseDescriptor::PrettyPrint(indent);
  result += std::string(" Type:") + PrettyPrintValue(audio_object_type_);
  result += std::string(" Freq:") +
      PrettyPrintValue(sampling_frequency_index_);
  result += std::string(" Channels:") + PrettyPrintValue(channel_config_);
  result += std::string(" ExtensionType:") +
      PrettyPrintValue(extension_audio_object_type_);
  result += std::string(" ExtensionFreq:") +
      PrettyPrintValue(extension_sampling_frequency_index_);
  result += std::string(" SBR:") + PrettyPrintValue(sbr_present_);
  return result;
}

uint32_t DecoderDescriptor::DecoderSpecificInfo::
    get_extension_sampling_frequency() const {
  // From ISO/IEC 14496-3:2005 Table 1.16.
  switch (extension_sampling_frequency_index_)  {
    case 0: return 96000;
    case 1: return 88200;
    case 2: return 64000;
    case 3: return 48000;
    case 4: return 44100;
    case 5: return 32000;
    case 6: return 24000;
    case 7: return 22050;
    case 8: return 16000;
    case 9: return 12000;
    case 10: return 11025;
    case 11: return 8000;
    case 12: return 7350;
  }
  // 13 and 14 are reserved, and we don't currently support 15.
  DASH_LOG("Unsupported extension_sampling_frequency", "", "");
  return 0;
}


// class DecoderConfigDescriptor extends BaseDescriptor :
//   bit(8) tag=DecoderConfigDescrTag {
//   bit(8) objectTypeIndication;
//   bit(6) streamType;
//   bit(1) upStream;
//   const bit(1) reserved=1;
//   bit(24) bufferSizeDB;
//   bit(32) maxBitrate;
//   bit(32) avgBitrate;
//   DecoderSpecificInfo decSpecificInfo[0 .. 1];
//   profileLevelIndicationIndexDescriptor
//        profileLevelIndicationIndexDescr [0..255];
// }
size_t DecoderDescriptor::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* bptr = buffer;
  bptr += ParseHeader(buffer, length);
  if (bptr == buffer) {
    return DashParser::kParseFailure;
  }
  if (get_tag() != kDecoderConfigDescrTag) {
    DASH_LOG("Expected Decoder Descriptor",
             "Wrong BaseDescriptor",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (!EnoughBytesToParse(bptr - buffer, kDecoderDescriptorMinSize, length)) {
    DASH_LOG("Elementary Stream Descriptor too short",
             "Need an id and flags",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  objectTypeIndication_ = *bptr;
  ++bptr;
  stream_type_ = (*bptr) >> kStreamTypeOffset;
  upstream_ = (((*bptr) >> kUpStreamOffset) & kUpStreamMask);
  ++bptr;
  // TODO(justsomeguy) Figure out how to handle a network byte order 24 bit
  // field.  Not really important, we don't use the value.
  buffer_size_db_ = 0;  // Skip for now.
  bptr += 3;
  max_bitrate_ = ntohlFromBuffer(bptr);
  bptr += sizeof(max_bitrate_);
  average_bitrate_ = ntohlFromBuffer(bptr);
  bptr += sizeof(average_bitrate_);
  size_t bytes_parsed = decoder_specific_info_.Parse(bptr,
                                                     length - (bptr - buffer));
  if (!bytes_parsed) {
    return DashParser::kParseFailure;
  }
  bptr += bytes_parsed;
  bytes_parsed = profile_descriptor_.Parse(bptr, length - (bptr - buffer));
  if (!bytes_parsed) {
    return DashParser::kParseFailure;
  }
  bptr += bytes_parsed;
  return bptr - buffer;
}

std::string DecoderDescriptor::PrettyPrint(std::string indent) const {
  std::string result = BaseDescriptor::PrettyPrint(indent);
  result += " ObjectType: " + PrettyPrintValue(objectTypeIndication_);
  result += " StreamType: " + PrettyPrintValue(stream_type_);
  if (upstream_) {
    result += " Upstream";
  }
  result += " BufferSize: " + PrettyPrintValue(buffer_size_db_);
  result += " Max BR: " + PrettyPrintValue(max_bitrate_);
  result += " Ave BR: " + PrettyPrintValue(average_bitrate_);
  result += "\n";
  result += decoder_specific_info_.PrettyPrint(indent + "  ") + "\n";
  result += profile_descriptor_.PrettyPrint(indent + "  ");
  return result;
}
}  // namespace dash2hls
