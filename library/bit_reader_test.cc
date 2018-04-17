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

#include "library/bit_reader.h"

#include <gtest/gtest.h>

namespace {
// binary value of s_bytes:  0b1010101011111111110011001101110111101110
const uint8_t s_bytes[] = {0xaa, 0xff, 0xcc, 0xdd, 0xee};
const uint8_t s_bytes64[] = {0xaa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
}  // namespace

namespace dash2hls {


// Each test below uses a pattern of bits to verify overflow and crossing byte
// boundaries works.  The choices are arbitrary as long as the following cases
// are checked:
//   Overflow size of value.
//   Read off the end of the buffer.
//   Read sizeof(value) when aligned.
//   Read sizeof(value) when misaligned.
//   Read less than the sizeof(value).

TEST(Dash2HLS, BitRead8) {
  BitReader reader(s_bytes, sizeof(s_bytes));
  uint8_t value;

  // Can't fit 9 bits in output:
  EXPECT_FALSE(reader.Read(9, &value));

  EXPECT_TRUE(reader.Read(1, &value));
  EXPECT_EQ(1, value);
  EXPECT_TRUE(reader.Read(1, &value));
  EXPECT_EQ(0, value);
  EXPECT_TRUE(reader.Read(8, &value));
  EXPECT_EQ(0b10101011, value);
  EXPECT_TRUE(reader.Read(7, &value));
  EXPECT_EQ(0b1111111, value);
  EXPECT_TRUE(reader.Read(7, &value));
  EXPECT_EQ(0b1001100, value);
  EXPECT_TRUE(reader.Read(7, &value));
  EXPECT_EQ(0b1101110, value);
  EXPECT_TRUE(reader.Read(7, &value));
  EXPECT_EQ(0b1111011, value);

  // Not enough bits left in buffer:
  EXPECT_FALSE(reader.Read(7, &value));
}

TEST(Dash2HLS, BitRead8_Exact) {
  BitReader reader(s_bytes, sizeof(s_bytes));
  uint8_t value;
  EXPECT_TRUE(reader.Read(8, &value));
  EXPECT_EQ(0xaa, value);
  EXPECT_TRUE(reader.Read(8, &value));
  EXPECT_EQ(0xff, value);
  EXPECT_TRUE(reader.Read(8, &value));
  EXPECT_EQ(0xcc, value);
  EXPECT_TRUE(reader.Read(8, &value));
  EXPECT_EQ(0xdd, value);
  EXPECT_TRUE(reader.Read(8, &value));
  EXPECT_EQ(0xee, value);
  EXPECT_FALSE(reader.Read(8, &value));
}

TEST(Dash2HLS, BitRead16) {
  BitReader reader(s_bytes, sizeof(s_bytes));
  uint16_t value;
  EXPECT_FALSE(reader.Read(17, &value));
  EXPECT_TRUE(reader.Read(1, &value));
  EXPECT_EQ(1, value);
  EXPECT_TRUE(reader.Read(1, &value));
  EXPECT_EQ(0, value);
  EXPECT_TRUE(reader.Read(8, &value));
  EXPECT_EQ(0b10101011, value);
  EXPECT_TRUE(reader.Read(13, &value));
  EXPECT_EQ(0b1111111100110, value);
  EXPECT_FALSE(reader.Read(32, &value));
}

TEST(Dash2HLS, BitRead32) {
  BitReader reader(s_bytes, sizeof(s_bytes));
  uint32_t value;
  EXPECT_FALSE(reader.Read(33, &value));
  EXPECT_TRUE(reader.Read(1, &value));
  EXPECT_EQ(0b1, value);
  EXPECT_TRUE(reader.Read(1, &value));
  EXPECT_EQ(0, value);
  EXPECT_TRUE(reader.Read(8, &value));
  EXPECT_EQ(0b10101011, value);
  EXPECT_TRUE(reader.Read(17, &value));
  EXPECT_EQ(0b11111111001100110, value);
  EXPECT_FALSE(reader.Read(32, &value));
}

TEST(Dash2HLS, BitRead64) {
  BitReader reader(s_bytes64, sizeof(s_bytes64));
  uint64_t value;
  EXPECT_FALSE(reader.Read(65, &value));
  EXPECT_TRUE(reader.Read(1, &value));
  EXPECT_EQ(1, value);
  EXPECT_TRUE(reader.Read(1, &value));
  EXPECT_EQ(0, value);
  EXPECT_TRUE(reader.Read(62, &value));
  EXPECT_EQ(0x2affffffffffffff, value);
  EXPECT_FALSE(reader.Read(1, &value));
}
}  // namespace dash2hls
