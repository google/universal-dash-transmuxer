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

#include <assert.h>

namespace {
inline uint8_t GetMask(uint8_t size) {
  return (1 << size) - 1;
}
}  // namespace

namespace dash2hls {

BitReader::BitReader(const uint8_t* data, size_t length)
    : data_(data), length_(length), bit_position_(0), byte_position_(0) {}

bool BitReader::Read(size_t bits_to_read, uint8_t* value) {
  return ReadInternal(bits_to_read, value);
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
  // The output is not large enough to hold the requested data.
  if (bits_to_read > sizeof(*value) * 8) {
    return false;
  }
  // The buffer does not contain the requested data.
  if (bits_to_read + bit_position_ + (byte_position_ * 8) > length_ * 8) {
    return false;
  }

  T result = 0;
  while (bits_to_read) {
    assert(byte_position_ < length_);

    const uint8_t current_byte = data_[byte_position_];
    const size_t bits_left_in_current_byte = 8 - bit_position_;

    // How many bits we will read in this pass of the loop:
    const size_t read_size = std::min(bits_to_read, bits_left_in_current_byte);

    // Calculate the new bits we are reading:
    const size_t shift_size = 8 - bit_position_ - read_size;
    const uint8_t mask = GetMask(read_size);
    const uint8_t new_bits = (current_byte >> shift_size) & mask;

    // Make room for the new bits, then add them to the result:
    result <<= read_size;
    result |= new_bits;

    // Track consumed bits:
    bits_to_read -= read_size;
    bit_position_ += read_size;

    // Move to the next byte if we've consumed the current one completely:
    assert(bit_position_ <= 8);
    if (bit_position_ == 8) {
      bit_position_ = 0;
      ++byte_position_;
    }
  }

  *value = result;
  return true;
}
}  // namespace dash2hls
