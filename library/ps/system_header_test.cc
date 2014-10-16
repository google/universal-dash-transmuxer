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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "library/ps/pes.h"
#include "library/utilities_gmock.h"

namespace {
// These test values are taken from the Widevine packager and live data.
const uint8_t kExpectedSystemHeader[] = {
  0x00, 0x00, 0x01, 0xbb, 0x00, 0x0f, 0x80, 0x00,
  0x01, 0x04, 0xe1, 0x7f, 0xbc, 0xe0, 0x00, 0xe0,
  0xe0, 0x00, 0xc0, 0xe0, 0x00};
}  // namespace

namespace dash2hls {

TEST(SystemHeader, BuildExpected) {
  SystemHeader header;
  header.AddStream(PES::kPsmStreamId);
  header.AddStream(PES::kVideoStreamId);
  header.AddStream(PES::kAudioStreamId);
  header.SetAudioBound(1);
  header.SetVideoBound(1);
  header.SetFixedFlag(false);
  header.SetCspsFlag(false);
  header.SetAudioLockFlag(true);
  header.SetVideoLockFlag(true);
  header.SetPacketRestrictionFlag(false);
  ASSERT_EQ(sizeof(kExpectedSystemHeader), header.GetSize());
  uint8_t buffer[sizeof(kExpectedSystemHeader)];
  EXPECT_EQ(sizeof(kExpectedSystemHeader),
            header.Write(buffer, sizeof(buffer)));
  EXPECT_THAT(std::make_pair(buffer, sizeof(buffer)),
              testing::MemEq(kExpectedSystemHeader,
                             sizeof(kExpectedSystemHeader)));
}
}  // namespace dash2hls
