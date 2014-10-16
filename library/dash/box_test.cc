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

#include "library/dash/box.h"

namespace {
// Sample box with a length of 10 and a trailing character.
const char kSimpleBox[] = "\0\0\0\012abcdxyz";
const size_t kExpectedBytesParsed = 10;
const size_t kSimpleBoxDataSize = 2;
}  // namespace

namespace dash2hls {

TEST(DashToHls, Box) {
  Box box(0);
  size_t bytes_parsed = box.Parse(reinterpret_cast<const uint8_t*>(kSimpleBox),
                                  sizeof(kSimpleBox));
  EXPECT_EQ(kExpectedBytesParsed, bytes_parsed);
  EXPECT_EQ(size_t(0), box.BytesNeededToContinue());
  EXPECT_EQ(true, box.DoneParsing());
}

TEST(DashToHls, BoxInChunks) {
  Box box(0);
  EXPECT_EQ(false, box.DoneParsing());
  EXPECT_EQ(sizeof(uint32_t), box.BytesNeededToContinue());
  size_t bytes_parsed = box.Parse(reinterpret_cast<const uint8_t*>(kSimpleBox),
                                  box.BytesNeededToContinue());
  EXPECT_EQ(sizeof(uint32_t), bytes_parsed);
  EXPECT_EQ(false, box.DoneParsing());

  EXPECT_EQ(sizeof(uint32_t), box.BytesNeededToContinue());
  bytes_parsed += box.Parse(reinterpret_cast<const uint8_t*>(kSimpleBox +
                                                             bytes_parsed),
                           box.BytesNeededToContinue());
  EXPECT_EQ(sizeof(uint32_t) * 2, bytes_parsed);
  EXPECT_EQ(false, box.DoneParsing());

  EXPECT_EQ(kSimpleBoxDataSize, box.BytesNeededToContinue());
  bytes_parsed += box.Parse(reinterpret_cast<const uint8_t*>(kSimpleBox +
                                                             bytes_parsed),
                            box.BytesNeededToContinue());
  EXPECT_EQ(kExpectedBytesParsed, bytes_parsed);
  EXPECT_EQ(size_t(0), box.BytesNeededToContinue());
  EXPECT_EQ(true, box.DoneParsing());
}
}  // namespace dash2hls
