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

#include "library/adts/adts_out.h"

#include "library/utilities.h"

namespace {
  const size_t kAudioFrameHeaderSize = 7;
  const size_t kMaxAudioFrameLength = 8191;
}  // namespace

namespace dash2hls {
  // The Pat and Pmt for TS are always the same.  There is no need to calculate
  // them.

const uint8_t AdtsOut::kID3AudioTimeTag[73] = {
  0x49, 0x44, 0x33, 0x03, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x3f, 0x50, 0x52, 0x49, 0x56, 0x00, 0x00,
  0x00, 0x35, 0x00, 0x00, 0x63, 0x6f, 0x6d, 0x2e,
  0x61, 0x70, 0x70, 0x6c, 0x65, 0x2e, 0x73, 0x74,
  0x72, 0x65, 0x61, 0x6d, 0x69, 0x6e, 0x67, 0x2e,
  0x74, 0x72, 0x61, 0x6e, 0x73, 0x70, 0x6f, 0x72,
  0x74, 0x53, 0x74, 0x72, 0x65, 0x61, 0x6d, 0x54,
  0x69, 0x6d, 0x65, 0x73, 0x74, 0x61, 0x6d, 0x70,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00
};
const size_t kID3AudioTimeTagTimeOffsetFromEnd = 8;

void AdtsOut::AddTimestamp(uint64_t pts, std::vector<uint8_t>* out) {
  size_t out_size = out->size();
  out->resize(out_size + sizeof(kID3AudioTimeTag));
  memcpy(&(*out)[out_size], kID3AudioTimeTag, sizeof(kID3AudioTimeTag));
  htonllToBuffer(pts,
                 &(*out)[out->size() - kID3AudioTimeTagTimeOffsetFromEnd]);
}

// TODO(justsomeguy) Code copied from the Widevine tree has quite a few
// magic numbers.  This is because it builds up the header by adding bits
// and shifting, adding bits and shifting.  I want to rewrite this to build
// in place and use constants.
void AdtsOut::ProcessSample(const uint8_t* input, size_t input_length,
                                       std::vector<uint8_t>* out) {
  size_t frame_size = input_length + kAudioFrameHeaderSize;
  if (frame_size > kMaxAudioFrameLength) {
    DASH_LOG("Bad audio sample",
             "Audio frames larger than 8191 bytes not supported.",
             DumpMemory(input, input_length).c_str());
    return;
  }
  out->resize(frame_size);
  uint64_t adts_header = 0xfff;
  adts_header <<= 4;
  adts_header |= 0x01;
  adts_header <<= 2;
  adts_header |= (audio_object_type_ - 1);
  adts_header <<= 4;
  adts_header |= sampling_frequency_index_;
  adts_header <<= 4;
  adts_header |= channel_config_;
  adts_header <<= 4;
  adts_header |= 0x0c;
  adts_header <<= 13;
  adts_header |= frame_size;
  adts_header <<= 11;
  adts_header |= 0x7ff;
  adts_header <<=2;
  adts_header <<= 8;
  htonllToBuffer(adts_header, &(*out)[0]);
  memcpy(&(*out)[kAudioFrameHeaderSize], input, input_length);
}
}  // namespace dash2hls
