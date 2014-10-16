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

#include "library/dash/box_type.h"
#include "library/dash/dash_parser.h"
#include "library/mac_test_files.h"
#include "library/dash/mvhd_contents.h"
#include "library/dash/sidx_contents.h"
#include "library/dash/trun_contents.h"
#include "library/utilities.h"

namespace {
// Sample box with a length of 10 and a trailing character.
const size_t kBufferSize = 1024;
// In a full fmp4 file there should be one sidx entry for each moof.  In
// the sample content the file is truncated to the first 8 moofs.
const size_t kExpectedMoofBoxes = 8;
const size_t kExpectedSidxEntries = 43;
const size_t kMvhdExpectedTimescale = 90000;
const size_t kTrunsExpected = 76;
}  // namespace

namespace dash2hls {

class Dash2HlsVideoTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    s_fp_ = Dash2HLS_GetTestVideoFile();
    ASSERT_NE(reinterpret_cast<FILE*>(0), s_fp_);
    size_t bytes_read = 0;
    do {
      uint8_t buffer[kBufferSize];
      bytes_read = fread(buffer, 1, kBufferSize, s_fp_);
      if (bytes_read) {
        ASSERT_NE(size_t(0), s_dash_parser_.Parse(buffer, bytes_read));
      }
    } while (bytes_read > 0);
  }
  static void TearDownTestCase() {
    if (s_fp_) {
      fclose(s_fp_);
    }
  }

 protected:
  static DashParser s_dash_parser_;
  static FILE* s_fp_;
};

class Dash2HlsAudioTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    s_fp_ = Dash2HLS_GetTestAudioFile();
    ASSERT_NE(reinterpret_cast<FILE*>(0), s_fp_);
    size_t bytes_read = 0;
    do {
      uint8_t buffer[kBufferSize];
      bytes_read = fread(buffer, 1, kBufferSize, s_fp_);
      if (bytes_read) {
        ASSERT_NE(size_t(0), s_dash_parser_.Parse(buffer, bytes_read));
      }
    } while (bytes_read > 0);
  }
  static void TearDownTestCase() {
    if (s_fp_) {
      fclose(s_fp_);
    }
  }

 protected:
  static DashParser s_dash_parser_;
  static FILE* s_fp_;
};

DashParser Dash2HlsVideoTest::s_dash_parser_;
FILE* Dash2HlsVideoTest::s_fp_;
DashParser Dash2HlsAudioTest::s_dash_parser_;
FILE* Dash2HlsAudioTest::s_fp_;

// The PrettyPrint format is for debugging and is a bit fragile.  It isn't
// worth testing for an actual value.  However, while debugging it is very
// helpful to dump out the PrettyPrint, so it's just commented out.
TEST_F(Dash2HlsVideoTest, PrettyPrint) {
  if (g_verbose_pretty_print) {
    printf("%s\n", s_dash_parser_.PrettyPrint("").c_str());
  }
}

TEST_F(Dash2HlsAudioTest, PrettyPrint) {
  if (g_verbose_pretty_print) {
    printf("%s\n", s_dash_parser_.PrettyPrint("").c_str());
  }
}

TEST_F(Dash2HlsVideoTest, Find) {
  const Box* a_box = s_dash_parser_.Find(BoxType::kBox_NoNe);
  EXPECT_EQ(nullptr, a_box);
  a_box = s_dash_parser_.Find(BoxType::kBox_moof);
  ASSERT_TRUE(a_box != nullptr);
  EXPECT_NE(nullptr, a_box);
  EXPECT_EQ(BoxType::kBox_moof, a_box->get_type().asUint32());
  a_box = s_dash_parser_.Find(BoxType::kBox_tfhd);
  EXPECT_EQ(nullptr, a_box);
}

TEST_F(Dash2HlsVideoTest, FindDeep) {
  const Box* a_box = s_dash_parser_.FindDeep(BoxType::kBox_NoNe);
  EXPECT_EQ(nullptr, a_box);
  a_box = s_dash_parser_.FindDeep(BoxType::kBox_moof);
  ASSERT_TRUE(a_box != nullptr);
  EXPECT_NE(nullptr, a_box);
  EXPECT_EQ(BoxType::kBox_moof, a_box->get_type().asUint32());
  a_box = s_dash_parser_.FindDeep(BoxType::kBox_tfhd);
  ASSERT_TRUE(a_box != nullptr);
  EXPECT_NE(nullptr, a_box);
}

TEST_F(Dash2HlsVideoTest, FindAll) {
  std::vector<const Box*> boxes;
  boxes = s_dash_parser_.FindAll(BoxType::kBox_NoNe);
  EXPECT_TRUE(boxes.empty());
  boxes = s_dash_parser_.FindAll(BoxType::kBox_moof);
  ASSERT_EQ(kExpectedMoofBoxes, boxes.size());
  EXPECT_EQ(BoxType::kBox_moof, boxes[0]->get_type().asUint32());
  boxes = s_dash_parser_.FindAll(BoxType::kBox_tfhd);
  EXPECT_TRUE(boxes.empty());
}

TEST_F(Dash2HlsVideoTest, FindDeepAll) {
  std::vector<const Box*> boxes;
  boxes = s_dash_parser_.FindDeepAll(BoxType::kBox_NoNe);
  EXPECT_TRUE(boxes.empty());
  boxes = s_dash_parser_.FindDeepAll(BoxType::kBox_moof);
  ASSERT_EQ(kExpectedMoofBoxes, boxes.size());
  EXPECT_EQ(BoxType::kBox_moof, boxes[0]->get_type().asUint32());
  boxes = s_dash_parser_.FindDeepAll(BoxType::kBox_tfhd);
  EXPECT_FALSE(boxes.empty());
  EXPECT_EQ(kExpectedMoofBoxes, boxes.size());
}

TEST_F(Dash2HlsVideoTest, Sidx) {
  const Box* box = s_dash_parser_.Find(BoxType::kBox_sidx);
  ASSERT_TRUE(box != nullptr);
  const SidxContents* sidx =
      reinterpret_cast<const SidxContents*>(box->get_contents());
  ASSERT_TRUE(sidx != nullptr);
  const std::vector<DashToHlsSegment> locations(sidx->get_locations());
  ASSERT_EQ(kExpectedSidxEntries, locations.size());
  for (uint32_t count = 0; count < kExpectedMoofBoxes; ++count) {
    uint8_t* buffer = new uint8_t[locations[count].length];
    fseek(s_fp_, locations[count].location, SEEK_SET);
    fread(buffer, 1, locations[count].length, s_fp_);
    DashParser parser;
    parser.Parse(buffer, locations[count].length);
    EXPECT_TRUE(parser.Find(BoxType::kBox_moof) != nullptr);
    EXPECT_TRUE(parser.FindDeep(BoxType::kBox_trun) != nullptr);
    EXPECT_TRUE(parser.Find(BoxType::kBox_mdat) != nullptr);
    delete[](buffer);
  }
}

TEST_F(Dash2HlsVideoTest, Mvhd) {
  const Box* box = s_dash_parser_.FindDeep(BoxType::kBox_mvhd);
  ASSERT_TRUE(box != nullptr);
  const MvhdContents* mvhd =
      reinterpret_cast<const MvhdContents*>(box->get_contents());
  ASSERT_TRUE(mvhd != nullptr);
  EXPECT_EQ(kMvhdExpectedTimescale, mvhd->get_timescale());
}

TEST_F(Dash2HlsVideoTest, Trun) {
  const Box* box = s_dash_parser_.Find(BoxType::kBox_sidx);
  ASSERT_TRUE(box != nullptr);
  const SidxContents* sidx =
      reinterpret_cast<const SidxContents*>(box->get_contents());
  ASSERT_TRUE(sidx != nullptr);
  const std::vector<DashToHlsSegment> locations(sidx->get_locations());
  ASSERT_LT(size_t(0), locations.size());

  std::vector<uint8_t> segment_bytes;
  segment_bytes.resize(locations[0].length);
  fseek(s_fp_, locations[0].location, SEEK_SET);
  fread(&segment_bytes[0], 1, locations[0].length, s_fp_);
  DashParser segment_parser;
  segment_parser.set_current_position(locations[0].location);
  segment_parser.Parse(&segment_bytes[0], segment_bytes.size());
  box = segment_parser.FindDeep(BoxType::kBox_trun);
  ASSERT_TRUE(box != nullptr);
  const TrunContents* trun =
      reinterpret_cast<const TrunContents*>(box->get_contents());
  ASSERT_TRUE(trun != nullptr);
  const std::vector<TrunContents::TrackRun>&
      track_runs(trun->get_track_runs());
  ASSERT_EQ(kTrunsExpected, track_runs.size());
}

}  // namespace dash2hls
