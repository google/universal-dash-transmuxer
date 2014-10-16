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

#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace {
const size_t kDecoderDescriptorMinSize = 14;
const size_t kStreamTypeOffset = 2;
const size_t kUpStreamOffset = 1;
const uint8_t kUpStreamMask = 0x1;
const size_t kDecoderSpecificInfoSize = 2;
const uint8_t kAudioObjectTypeOffset = 3;
const uint8_t kSamplingFrequencyIndex1Offset = 1;
const uint8_t kSamplingFrequencyIndex1Mask = 0x07;
const uint8_t kSamplingFrequencyIndex2Offset = 7;
const uint8_t kSamplingFrequencyIndex2Mask = 0x01;
const uint8_t kChannelsOffset = 3;
const uint8_t kChannelsMask = 0x0f;
}  // namespace

namespace dash2hls {

size_t DecoderDescriptor::DecoderSpecificInfo::Parse(const uint8_t* buffer,
                                                     size_t length) {
  size_t bytes_parsed = BaseDescriptor::ParseHeader(buffer, length);
  if (bytes_parsed == 0) {
    return DashParser::kParseFailure;
  }
  const uint8_t* bptr = buffer + bytes_parsed;
  if (get_size() != kDecoderSpecificInfoSize) {
    DASH_LOG("Bad DecoderSpecificInfo",
             "Expected exactly 2 bytes.",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (get_tag() != kDecSpecificInfoTag) {
    DASH_LOG("Bad DecoderSpecificInfo",
             "Expected kDecSpecificInfoTag.",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  audio_object_type_ = bptr[0] >> kAudioObjectTypeOffset;
  sampling_frequency_index_ =
      ((bptr[0] & kSamplingFrequencyIndex1Mask) <<
       kSamplingFrequencyIndex1Offset) |
      ((bptr[1] >> kSamplingFrequencyIndex2Offset) &
       kSamplingFrequencyIndex2Mask);
  channel_config_ = (bptr[1] >> kChannelsOffset) & kChannelsMask;
  audio_config_[0] = bptr[0];
  audio_config_[1] = bptr[1];
  bptr += kDecoderSpecificInfoSize;
  return bptr - buffer;
}

std::string
DecoderDescriptor::DecoderSpecificInfo::PrettyPrint(std::string indent) const {
  std::string result = BaseDescriptor::PrettyPrint(indent);
  result += std::string(" Type:") + PrettyPrintValue(audio_object_type_);
  result += std::string(" Freq:") +
      PrettyPrintValue(sampling_frequency_index_);
  result += std::string(" Channels:") + PrettyPrintValue(channel_config_);
  return result;
}

// class DecoderConfigDescriptor extends BaseDescriptor :
//     bit(8) tag=DecoderConfigDescrTag {
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
