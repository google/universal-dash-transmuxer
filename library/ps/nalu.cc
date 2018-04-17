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

#include "library/ps/nalu.h"

#include <string.h>

#include "library/dash/dash_parser.h"

namespace dash2hls {
namespace nalu {

// Reads an integer of size |nalu_length_size| bytes from |nalu|.
size_t GetLength(const uint8_t* nalu, size_t buffer_length,
                 size_t nalu_length_size) {
  if (buffer_length < nalu_length_size) {
    return DashParser::kParseFailure;
  }
  size_t result = 0;
  while (nalu_length_size-- > 0) {
    result <<= 8;
    result |= *nalu;
    ++nalu;
  }
  return result;
}

namespace {
bool AdvanceBit(const uint8_t** buffer, uint32_t* buffer_size,
                uint8_t* current_byte, uint32_t* bit_offset) {
  *current_byte <<=1;
  ++(*bit_offset);
  if (*bit_offset == 8) {
    if (*buffer_size > 1) {
      --(*buffer_size);
      *bit_offset = 0;
      ++(*buffer);
      *current_byte = (*buffer)[0];
      return true;
    } else {
      return false;
    }
  }
  return true;
}
}  // namespace

int32_t ReadBitsLength(const uint8_t** buffer, uint32_t* buffer_size,
                       uint8_t* current_byte, uint32_t* bit_offset) {
  uint32_t number_of_bits = 0;
  while (*buffer_size > 0 && !(*current_byte & 0x80)) {
    ++number_of_bits;
    if (!AdvanceBit(buffer, buffer_size, current_byte, bit_offset)) {
      return -1;
    }
  }
  if (!AdvanceBit(buffer, buffer_size, current_byte, bit_offset)) {
    return -1;
  }

  uint32_t value = 0;
  for (size_t count = 0; count < number_of_bits; ++count) {
    value <<= 1;
    value |= ((*current_byte & 0x80) >> 7);
    if (!AdvanceBit(buffer, buffer_size, current_byte, bit_offset)) {
      return -1;
    }
  }
  value += (uint32_t(1) << number_of_bits) - 1;
  return value;
}

int32_t GetSliceType(const uint8_t* nalu_buffer, size_t buffer_size) {
  if (buffer_size < kNaluHeaderSize + 1) {
    return -1;
  }
  const uint8_t* slice = nalu_buffer + 1;
  uint32_t slice_size = static_cast<uint32_t>(buffer_size - 1);
  uint8_t current_byte = slice[0];
  uint32_t bit_offset = 0;
  int32_t first_mb = ReadBitsLength(&slice, &slice_size, &current_byte,
                                    &bit_offset);
  if (first_mb < 0) {
    return -1;
  }
  return ReadBitsLength(&slice, &slice_size, &current_byte, &bit_offset);
}

namespace {
// See ISO-24496-10 for these magic numbers.
PicType PicTypeFromFrameTypes(bool frame_types[5]) {
  if (frame_types[1]) {  // B-Frames
    return (frame_types[3] || frame_types[4]) ?
        kPicType_I_SI_P_SP_B:kPicType_IPB;
  }
  if (frame_types[0]) {  // P-Frames
    return (frame_types[3] || frame_types[4]) ? kPicType_I_SI_P_SP:kPicType_IP;
  }
  if (frame_types[2]) {  // I-Frames
    return frame_types[4] ? kPicType_I_SI:kPicType_I;
  }
  if (frame_types[4]) {  // SI slices
    return frame_types[3] ? kPicType_SI_SP:kPicType_SI;
  }
  return kPicType_Error;
}
}  // namespace

size_t PreprocessNalus(uint8_t* nalu_buffer, size_t buffer_size,
                       size_t nalu_length_size, bool* has_aud,
                       PicType* pic_type) {
  bool frame_types[5] = {false, false, false, false, false};
  *has_aud = false;
  uint8_t* nalu_end = nalu_buffer + buffer_size;
  uint8_t* read_ptr = nalu_buffer;
  uint8_t* write_ptr = nalu_buffer;
  while (read_ptr < nalu_end) {
    size_t length = GetLength(read_ptr, nalu_end - read_ptr,
                              nalu_length_size);
    if (!length || (read_ptr + length + nalu_length_size > nalu_end)) {
      return DashParser::kParseFailure;
    }
    uint8_t nalu_type = read_ptr[nalu_length_size] & kNaluTypeMask;
    if (nalu_type != kNaluType_Filler) {
      // Only need to copy bytes if a padding nalu has been skipped.
      if (read_ptr > write_ptr) {
        // IMPORTANT!  Use memmove, because the ranges MAY overlap!
        memmove(write_ptr, read_ptr, nalu_length_size + length);
      }
      write_ptr += nalu_length_size + length;
      switch (nalu_type) {
        case kNaluType_UnpartitionedNonIdrSlice:
        case kNaluType_IdrSlice:
        case kNaluType_UnpartitionedAuxiliaryPictureSlice:
          {
            uint32_t slice_type =
                GetSliceType(read_ptr + nalu_length_size,
                             nalu_end - read_ptr - nalu_length_size);
            // See ISO 14496-10 section 7.4.3 for details.
            // Slices 5-9 are really slices 0-4 for our purposes.
            if (slice_type >= 5) {
              slice_type -= 5;
            }
            if (slice_type < 5) {
              frame_types[slice_type] = true;
            }
          }
          break;
        case kNaluType_AuDelimiter:
          *has_aud = true;
          break;
        default:
          break;
      }
    }  // if (nalu_type != kNaluTypefiller)
    read_ptr += nalu_length_size + length;
    if (read_ptr > nalu_end) {
      return DashParser::kParseFailure;
    }
  }
  *pic_type = PicTypeFromFrameTypes(frame_types);
  return write_ptr - nalu_buffer;
}

bool ReplaceLengthWithStartCode(uint8_t* nalu_buffer, size_t buffer_size) {
  uint8_t* nalu_end = nalu_buffer + buffer_size;
  uint8_t* read_ptr = nalu_buffer;
  while (read_ptr < nalu_end) {
    size_t length = GetLength(read_ptr, nalu_end - read_ptr, sizeof(uint32_t));
    if (!length || (read_ptr + length > nalu_end)) {
      return false;
    }
    // We know we have the 4 bytes because Length succeeded.
    memcpy(read_ptr, "\000\000\000\001", sizeof(uint32_t));
    read_ptr += sizeof(uint32_t) + length;
    if (read_ptr > nalu_end) {
      return false;
    }
  }
  return true;
}

}  // namespace nalu
}  // namespace dash2hls
