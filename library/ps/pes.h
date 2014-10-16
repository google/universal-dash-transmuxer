#ifndef _DASH2HLS_PES_H_
#define _DASH2HLS_PES_H_

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

// Every sample is encoded in a PES packet.
// See ISO-14966-10 for details on its internals.

#include <stdint.h>
#include <string>
#include <vector>

namespace dash2hls {

class PES {
 public:
  static const uint8_t kPesStartCode[3];
  enum {
    kPsmStreamId = 0xbc,
    kAudioStreamId = 0xc0,
    kVideoStreamId = 0xe0,
    kVideoStreamType = 0x1b,
    kAudioStreamType = 0x0f,
  };

 public:
  PES();

  size_t GetSize() const;
  size_t GetHeaderSize() const;
  void set_max_size(size_t size) {max_size_ = size;}

  size_t Write(uint8_t* buffer, size_t max_length) const;
  size_t WritePartial(uint8_t* buffer, size_t start, size_t length) const;
  size_t WriteHeader(uint8_t* buffer, size_t max_length) const;

  void AddPayload(const uint8_t* payload, size_t length) {
    payload_ = payload;
    payload_size_ = length;
  }
  size_t GetFreePayloadBytes() const;

  void SetCopyright(bool copyright);
  void SetOriginal(bool original);
  void SetScramblingBits(uint8_t scrambling_bits);
  void SetDataAlignmentIndicator(bool aligned);
  void SetExtensionFlags(uint8_t flags1, uint8_t flags2) {
    flags1_ = flags1;
    flags2_ = flags2;
  }
  bool HasOptHeader() const;

  void SetPts(uint64_t pts);
  void SetDts(uint64_t dts);
  void set_stream_id(uint32_t stream_id) {stream_id_ = stream_id;}

 private:
  size_t max_size_;
  uint8_t stream_id_;
  uint8_t flags1_;
  uint8_t flags2_;
  uint64_t pts_;
  uint64_t dts_;
  const uint8_t* payload_;
  size_t payload_size_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_PES_H_
