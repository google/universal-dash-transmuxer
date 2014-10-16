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

#include "library/ps/system_header.h"

#include <string.h>
#include "library/utilities.h"

namespace {
const uint32_t kDefaultFlags1 = 0x00;
const uint32_t kDefaultFlags2 = 0x20;
const uint32_t kDefaultFlags3 = 0x7f;
const size_t kSystemHeaderBaseSize = 12;
const size_t kStreamSize = 3;
const uint8_t kSystemHeaderStartCode[4] = {0x00, 0x00, 0x01, 0xBB};
const uint32_t kRateBoundShiftBits = 9;
const uint32_t kRateBoundMask = 0x80000100;
const uint32_t kAudioBoundShiftBits = 2;
const uint32_t kStreamSizeBoundBitSet = 0xc000;
const uint32_t kStreamSizeBoundMask1 = 0x1fff;
const uint32_t kStreamSizeBoundMask2 = 0x2000;
const uint32_t kStreamSizeBoundBitShift2 = 2;
const uint32_t kFixedFlagMask = 0x02;
const uint32_t kCspsFlagMask = 0x01;
const uint32_t kAudioLockFlagMask = 0x80;
const uint32_t kVideoLockFlagMask = 0x40;
const uint32_t kPacketRestrictionFlagMask = 0x80;
const uint32_t kBufferSizeScaleDefaultValue = 0x8000;
}  // namespace

namespace dash2hls {

SystemHeader::SystemHeader()
    : rate_bound_(0),
      audio_bound_(0),
      video_bound_(0),
      flags1_(kDefaultFlags1),
      flags2_(kDefaultFlags2),
      flags3_(kDefaultFlags3) {
}

uint32_t SystemHeader::GetSize() const {
  return static_cast<uint32_t>(kSystemHeaderBaseSize +
                                   (streams_.size() * kStreamSize));
}

uint32_t SystemHeader::Write(uint8_t* buffer, uint32_t max_length) const {
  uint32_t size = GetSize();
  if (size > max_length) {
    return(0);
  }

  memcpy(buffer, kSystemHeaderStartCode, sizeof(kSystemHeaderStartCode));
  buffer += sizeof(kSystemHeaderStartCode);
  htonsToBuffer(size - 6, buffer);
  buffer += sizeof(uint16_t);
  htonlToBuffer((rate_bound_ << kRateBoundShiftBits) | kRateBoundMask,
                buffer);
  buffer += sizeof(uint32_t) - 1;
  *buffer = (audio_bound_ << kAudioBoundShiftBits) | flags1_;
  ++buffer;
  *buffer = flags2_ | video_bound_;
  ++buffer;
  *buffer = flags3_;
  ++buffer;
  for (size_t count = 0; count < streams_.size(); ++count) {
    *buffer = streams_[count].first;
    ++buffer;
    htonsToBuffer(kStreamSizeBoundBitSet |
                  (streams_[count].second & kStreamSizeBoundMask1) |
                  ((streams_[count].second >> kStreamSizeBoundBitShift2)
                   & kStreamSizeBoundMask2), buffer);
    buffer += 2;
  }

  return size;
}

void SystemHeader::SetFixedFlag(bool flag) {
  if (flag) {
    flags1_ |= kFixedFlagMask;
  } else {
    flags1_ &= ~kFixedFlagMask;
  }
}

void SystemHeader::SetCspsFlag(bool flag) {
  if (flag) {
    flags1_ |= kCspsFlagMask;
  } else {
    flags1_ &= ~kCspsFlagMask;
  }
}

void SystemHeader::SetAudioLockFlag(bool flag) {
  if (flag) {
    flags2_ |= kAudioLockFlagMask;
  } else {
    flags2_ &= ~kAudioLockFlagMask;
  }
}

void SystemHeader::SetVideoLockFlag(bool flag) {
  if (flag) {
    flags2_ |= kVideoLockFlagMask;
  } else {
    flags2_ &= ~kVideoLockFlagMask;
  }
}

void SystemHeader::SetPacketRestrictionFlag(bool flag) {
  if (flag) {
    flags3_ |= kPacketRestrictionFlagMask;
  } else {
    flags3_ &= ~kPacketRestrictionFlagMask;
  }
}

void SystemHeader::AddStream(uint8_t stream_id) {
  streams_.push_back(std::make_pair(stream_id, kBufferSizeScaleDefaultValue));
}

void SystemHeader::SetBufferSizeBound(uint8_t stream_id,
                                      uint8_t buffer_size_scale,
                                      uint16_t buffer_size_bound) {
  for (size_t count = 0; count < streams_.size(); ++count) {
    if (streams_[count].first == stream_id) {
      streams_[count].second = buffer_size_bound |
          (buffer_size_scale ? 0 : kBufferSizeScaleDefaultValue);
      break;
    }
  }
}
}  // namespace dash2hls
