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

#include "bit_reader.h"

#include "utilities.h"

namespace {
uint8_t GetMask(uint8_t size) {
  return (1 << size) - 1;
}
}  // namespace

namespace dash2hls {

BitReader::BitReader(const uint8_t* bits, size_t length): bits_(bits),
    length_(length), bit_position_(0), byte_position_(0) {
}

bool BitReader::Read(size_t bits_to_read, uint8_t* value) {
  if (bits_to_read > sizeof(*value) * 8) {
    return false;
  }
  if (bits_to_read + bit_position_ + (byte_position_ * 8) > length_ * 8) {
    return false;
  }

  if (bits_to_read + bit_position_ < 8) {
    *value = (bits_[byte_position_] >> (8 - bit_position_ - bits_to_read))
        & GetMask(bits_to_read);
    bit_position_ += bits_to_read;
    return true;
  }
  size_t msb_to_read = 8 - bit_position_;
  size_t lsb_to_read = bits_to_read - msb_to_read;
  *value = ((bits_[byte_position_] & GetMask(msb_to_read)) <<
                (bits_to_read - msb_to_read)) |
           ((bits_[byte_position_ + 1] >> (8 - lsb_to_read))
                & GetMask(lsb_to_read));
  bit_position_ = lsb_to_read;
  ++byte_position_;
  return true;
}

bool BitReader::Read(size_t bits_to_read, uint16_t* value) {
  return ReadInternal(bits_to_read, value);
}

bool BitReader::Read(size_t bits_to_read, uint32_t* value) {
  return ReadInternal(bits_to_read, value);
}

bool BitReader::Read(size_t bits_to_read, uint64_t* value) {
  return ReadInternal(bits_to_read, value);
}

template <typename T> bool BitReader::ReadInternal(size_t bits_to_read,
                                                   T* value) {
  if (bits_to_read > sizeof(*value) * 8) {
    return false;
  }
  if (bits_to_read + bit_position_ + (byte_position_ * 8) > length_ * 8) {
    return false;
  }

  T result = 0;
  while (bits_to_read > 8) {
    result <<= 8;
    uint8_t next_byte = 0;
    if (!Read(8, &next_byte)) {
      return false;
    }
    result += next_byte;
    bits_to_read -= 8;
  }
  result <<= bits_to_read;
  uint8_t next_byte = 0;
  if (!Read(bits_to_read, &next_byte)) {
    return false;
  }
  result += next_byte;
  *value = result;
  return true;
}
}  // namespace dash2hls
