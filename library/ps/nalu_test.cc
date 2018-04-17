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

#include <gtest/gtest.h>

#include "library/utilities_gmock.h"
#include "library/ps/nalu.h"

namespace {
// kFillerNalu was hand crafted, the rest are pulled from real content.
// The only nalu we should see are 1, 5, and 6 but it's possible to get 12.

// nalu 1.
const uint8_t kUnpartitionedNonIdrSlice[] = {
  0x00, 0x00, 0x00, 0x22, 0x21, 0xe1, 0x04, 0x03,
  0xe0, 0x25, 0x4d, 0xc0, 0x7f, 0x00, 0x0e, 0xc2,
  0x10, 0x30, 0x40, 0x8f, 0x2d, 0x53, 0x47, 0xd3,
  0x6f, 0x1a, 0x41, 0x92, 0x5b, 0xb8, 0x97, 0xc2,
  0x9f, 0xfd, 0xb6, 0xf6, 0xdb, 0xe0};
const size_t kUnpartitionedNonIdrSliceLength =
    sizeof(kUnpartitionedNonIdrSlice) - sizeof(uint32_t);

// nalu 5.
const uint8_t kIdrSliceNalu[] = {
  0x00, 0x00, 0x00, 0x87, 0x25, 0xb8, 0x20, 0x00,
  0xf9, 0x31, 0x40, 0x00, 0x52, 0xea, 0xfb, 0xef,
  0xbe, 0xfb, 0xef, 0xbe, 0xfb, 0xef, 0xbe, 0xfb,
  0xef, 0xbe, 0xfb, 0xef, 0xae, 0xba, 0xeb, 0xae,
  0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb,
  0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba,
  0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae,
  0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb,
  0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba,
  0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae,
  0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb,
  0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba,
  0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae,
  0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb,
  0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba,
  0xeb, 0xae, 0xba, 0xeb, 0xaf, 0xff, 0xf9, 0xf0,
  0xa0, 0x43, 0x80, 0x91, 0xeb, 0x7e, 0x5a, 0xbb,
  0x9b, 0xf3, 0xf0};
const size_t kIdrSliceNaluLength = sizeof(kIdrSliceNalu) - sizeof(uint32_t);

// nalu 6.
const uint8_t kSupEnhInfoNalu[] = {
  0x00, 0x00, 0x00, 0x0b, 0x06, 0x00, 0x07, 0x81,
  0xf6, 0x3b, 0x80, 0x00, 0x00, 0x40, 0x80};
const size_t kSupEnhInfoNaluLength = sizeof(kSupEnhInfoNalu) -
    sizeof(uint32_t);

// nalu 9.
const uint8_t kAudNalu[] = {
  0x00, 0x00, 0x00, 0x02, 0x09, 0x00
};

// nalu 9.
const uint8_t kAudNaluStartCode[] = {
  0x00, 0x00, 0x00, 0x01, 0x09, 0x00
};

// nalu 12.
const uint8_t kFillerNalu[] = {0, 0, 0, 4, dash2hls::nalu::kNaluType_Filler,
                               0xff, 0xff, 0xff};
const size_t kFillerNalulength = sizeof(kFillerNalu) - sizeof(uint32_t);
const uint8_t kFillerNaluTruncated[] = {
  0, 0, 0, 4, dash2hls::nalu::kNaluType_Filler, 0xff, 0xff};
const uint8_t kFillerNaluBadLength[] = {0, 0, 0};

const size_t kNaluLengthSize = 4;

// length encoded 0-9
const uint8_t kReadBitsVector[] = {0xa6, 0x42, 0x98, 0xe2, 0x04, 0x8a};
const uint8_t kReadBitsShortVector1[] = {0x00};
const uint8_t kReadBitsShortVector2[] = {0x08};
}  // namespace

namespace dash2hls {

TEST(DashToHlsPs, NaluLength) {
  EXPECT_EQ(kUnpartitionedNonIdrSliceLength,
            nalu::GetLength(kUnpartitionedNonIdrSlice,
                            sizeof(kUnpartitionedNonIdrSlice),
                            kNaluLengthSize));
  EXPECT_EQ(kIdrSliceNaluLength,
            nalu::GetLength(kIdrSliceNalu, sizeof(kIdrSliceNalu),
                            kNaluLengthSize));
  EXPECT_EQ(kSupEnhInfoNaluLength,
            nalu::GetLength(kSupEnhInfoNalu, sizeof(kSupEnhInfoNalu),
                            kNaluLengthSize));
  EXPECT_EQ(kFillerNalulength,
            nalu::GetLength(kFillerNalu, sizeof(kFillerNalu),
                            kNaluLengthSize));
  EXPECT_EQ(kFillerNalulength,
            nalu::GetLength(kFillerNaluTruncated, sizeof(kFillerNaluTruncated),
                            kNaluLengthSize));
  EXPECT_EQ(size_t(0), nalu::GetLength(kFillerNaluBadLength,
                                       sizeof(kFillerNaluBadLength),
                                       kNaluLengthSize));
}

TEST(DashToHlsPs, ReadBitsLength) {
  const uint8_t* bytes = kReadBitsVector;
  uint32_t bit_offset = 0;
  uint32_t bytes_size = sizeof(kReadBitsVector);
  uint8_t current_byte = bytes[0];
  for (int32_t count = 0; count < 9; ++count) {
    EXPECT_EQ(count, nalu::ReadBitsLength(&bytes, &bytes_size,
                                          &current_byte, &bit_offset));
  }

  bytes = kReadBitsShortVector1;
  bit_offset = 0;
  bytes_size = sizeof(kReadBitsShortVector1);
  current_byte = bytes[0];
  EXPECT_EQ(-1, nalu::ReadBitsLength(&bytes, &bytes_size,
                                    &current_byte, &bit_offset));


  bytes = kReadBitsShortVector2;
  bit_offset = 0;
  bytes_size = sizeof(kReadBitsShortVector2);
  current_byte = bytes[0];
  EXPECT_EQ(-1, nalu::ReadBitsLength(&bytes, &bytes_size,
                                    &current_byte, &bit_offset));
}

TEST(DashToHlsPs, GetSliceType) {
  int32_t result = nalu::GetSliceType(kIdrSliceNalu, sizeof(kIdrSliceNalu));
  EXPECT_EQ(2, result);
  result = nalu::GetSliceType(kIdrSliceNalu, sizeof(uint32_t));
  EXPECT_EQ(-1, result);
}

TEST(DashToHlsPs, PreprocessNalus) {
  // Allocate a buffer big enough for all the tests.
  uint8_t buffer[2 * sizeof(kIdrSliceNalu) + 3 * sizeof(kFillerNalu)];
  bool has_aud;
  nalu::PicType pic_type;

  // padding first.
  memcpy(buffer, kFillerNalu, sizeof(kFillerNalu));
  memcpy(buffer + sizeof(kFillerNalu), kIdrSliceNalu, sizeof(kIdrSliceNalu));
  size_t new_size = nalu::PreprocessNalus(buffer, sizeof(kIdrSliceNalu) +
                                          sizeof(kFillerNalu), kNaluLengthSize,
                                          &has_aud, &pic_type);
  EXPECT_EQ(sizeof(kIdrSliceNalu), new_size);
  EXPECT_THAT(std::make_pair(buffer, sizeof(kIdrSliceNalu)),
              testing::MemEq(kIdrSliceNalu, sizeof(kIdrSliceNalu)));
  EXPECT_EQ(false, has_aud);

  // padding last.
  memcpy(buffer, kIdrSliceNalu, sizeof(kIdrSliceNalu));
  memcpy(buffer + sizeof(kIdrSliceNalu), kFillerNalu, sizeof(kFillerNalu));
  new_size = nalu::PreprocessNalus(buffer, sizeof(kIdrSliceNalu) +
                                   sizeof(kFillerNalu), kNaluLengthSize,
                                   &has_aud, &pic_type);
  EXPECT_EQ(sizeof(kIdrSliceNalu), new_size);

  // padding in the middle.
  memcpy(buffer, kIdrSliceNalu, sizeof(kIdrSliceNalu));
  memcpy(buffer + sizeof(kIdrSliceNalu), kFillerNalu, sizeof(kFillerNalu));
  memcpy(buffer + sizeof(kFillerNalu) + sizeof(kIdrSliceNalu),
         kIdrSliceNalu, sizeof(kIdrSliceNalu));
  new_size = nalu::PreprocessNalus(buffer, sizeof(kIdrSliceNalu) * 2 +
                                   sizeof(kFillerNalu), kNaluLengthSize,
                                   &has_aud, &pic_type);
  EXPECT_EQ(sizeof(kIdrSliceNalu) * 2, new_size);
  EXPECT_THAT(std::make_pair(buffer, sizeof(kIdrSliceNalu)),
              testing::MemEq(kIdrSliceNalu, sizeof(kIdrSliceNalu)));
  EXPECT_THAT(std::make_pair(buffer + sizeof(kIdrSliceNalu),
                             sizeof(kIdrSliceNalu)),
              testing::MemEq(kIdrSliceNalu, sizeof(kIdrSliceNalu)));
  EXPECT_EQ(false, has_aud);

  // multiple padding.
  memcpy(buffer, kFillerNalu, sizeof(kFillerNalu));
  memcpy(buffer + sizeof(kFillerNalu), kIdrSliceNalu, sizeof(kIdrSliceNalu));
  memcpy(buffer + sizeof(kFillerNalu) + sizeof(kIdrSliceNalu),
         kFillerNalu, sizeof(kFillerNalu));
  memcpy(buffer + sizeof(kFillerNalu) * 2 + sizeof(kIdrSliceNalu),
         kIdrSliceNalu, sizeof(kIdrSliceNalu));
  memcpy(buffer + sizeof(kFillerNalu) * 2 + sizeof(kIdrSliceNalu) * 2,
         kFillerNalu, sizeof(kFillerNalu));
  new_size = nalu::PreprocessNalus(buffer, sizeof(kIdrSliceNalu) * 2 +
                                   sizeof(kFillerNalu) * 3, kNaluLengthSize,
                                   &has_aud, &pic_type);
  EXPECT_EQ(sizeof(kIdrSliceNalu) * 2, new_size);
  EXPECT_THAT(std::make_pair(buffer, sizeof(kIdrSliceNalu)),
              testing::MemEq(kIdrSliceNalu, sizeof(kIdrSliceNalu)));
  EXPECT_THAT(std::make_pair(buffer + sizeof(kIdrSliceNalu),
                             sizeof(kIdrSliceNalu)),
              testing::MemEq(kIdrSliceNalu, sizeof(kIdrSliceNalu)));
  EXPECT_EQ(false, has_aud);

  // bad sizes
  memcpy(buffer, kFillerNalu, sizeof(kFillerNalu));
  memcpy(buffer + sizeof(kFillerNalu), kIdrSliceNalu, sizeof(kIdrSliceNalu));
  new_size = nalu::PreprocessNalus(buffer, sizeof(kFillerNalu) - 1,
                                   kNaluLengthSize,  &has_aud, &pic_type);
  EXPECT_EQ(size_t(0), new_size);
  memcpy(buffer, kFillerNalu, sizeof(kFillerNalu));
  new_size = nalu::PreprocessNalus(buffer, sizeof(kFillerNalu) +2,
                                   kNaluLengthSize,  &has_aud, &pic_type);
  EXPECT_EQ(size_t(0), new_size);
  EXPECT_EQ(false, has_aud);

  // Aud detection.
  memcpy(buffer, kAudNalu, sizeof(kAudNalu));
  new_size = nalu::PreprocessNalus(buffer, sizeof(kAudNalu),
                                   kNaluLengthSize,  &has_aud, &pic_type);
  EXPECT_EQ(sizeof(kAudNalu), new_size);
  EXPECT_EQ(true, has_aud);

  memcpy(buffer, kFillerNalu, sizeof(kFillerNalu));
  memcpy(buffer + sizeof(kFillerNalu), kAudNalu, sizeof(kAudNalu));
  new_size = nalu::PreprocessNalus(buffer,
                                   sizeof(kFillerNalu) + sizeof(kAudNalu),
                                   kNaluLengthSize,  &has_aud, &pic_type);
  EXPECT_EQ(sizeof(kAudNalu), new_size);
  EXPECT_EQ(true, has_aud);

  memcpy(buffer, kFillerNalu, sizeof(kFillerNalu));
  memcpy(buffer + sizeof(kFillerNalu), kAudNalu, sizeof(kAudNalu));
  memcpy(buffer + sizeof(kFillerNalu) + sizeof(kAudNalu),
         kFillerNalu, sizeof(kFillerNalu));
  new_size = nalu::PreprocessNalus(buffer,
                                   2 * sizeof(kFillerNalu) + sizeof(kAudNalu),
                                   kNaluLengthSize,  &has_aud, &pic_type);
  EXPECT_EQ(sizeof(kAudNalu), new_size);
  EXPECT_EQ(true, has_aud);

  // TODO(justsomeguy) test pic_type
}

TEST(DashToHlsPs, ReplaceLengthWithStartCode) {
  uint8_t input[sizeof(kAudNalu)*3];
  uint8_t expected_output[sizeof(kAudNaluStartCode)*3];

  memcpy(input, kAudNalu, sizeof(kAudNalu));
  memcpy(expected_output, kAudNaluStartCode, sizeof(kAudNaluStartCode));
  memcpy(input + sizeof(kAudNalu), kAudNalu, sizeof(kAudNalu));
  memcpy(expected_output + sizeof(kAudNaluStartCode),
         kAudNaluStartCode, sizeof(kAudNaluStartCode));
  memcpy(input + sizeof(kAudNalu) * 2, kAudNalu, sizeof(kAudNalu));
  memcpy(expected_output + sizeof(kAudNaluStartCode) * 2,
         kAudNaluStartCode, sizeof(kAudNaluStartCode));

  bool success = nalu::ReplaceLengthWithStartCode(input, sizeof(input));
  EXPECT_TRUE(success);
  EXPECT_THAT(std::make_pair(input, sizeof(input)),
              testing::MemEq(expected_output, sizeof(expected_output)));

  success = nalu::ReplaceLengthWithStartCode(input, sizeof(input) - 1);
  EXPECT_FALSE(success);
}
}  // namespace dash2hls
