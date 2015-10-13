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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "include/DashToHlsApi.h"
#include "library/dash/avcc_contents.h"
#include "library/dash/box_type.h"
#include "library/dash/dash_parser.h"
#include "library/dash/mdat_contents.h"
#include "library/dash/saio_contents.h"
#include "library/dash/saiz_contents.h"
#include "library/dash/tenc_contents.h"
#include "library/dash/tfdt_contents.h"
#include "library/dash/tfhd_contents.h"
#include "library/dash/trex_contents.h"
#include "library/dash/trun_contents.h"
#include "library/dash_to_hls_session.h"
#include "library/mac_test_files.h"
#include "library/utilities.h"
#include "library/utilities_gmock.h"

namespace {
const uint8_t kAvcCBox[] = {0x00, 0x00, 0x00, 0x2d, 0x61, 0x76, 0x63, 0x43,
                            0x01, 0x42, 0xe0, 0x0d, 0xff, 0xe1, 0x00, 0x16,
                            0x27, 0x42, 0xe0, 0x0d, 0xa9, 0x18, 0x28, 0x3f,
                            0x60, 0x0d, 0x41, 0x80, 0x41, 0xad, 0xb7, 0xa0,
                            0x2f, 0x01, 0xe9, 0x7b, 0xdf, 0x01, 0x01, 0x00,
                            0x04, 0x28, 0xce, 0x09, 0x88};
const uint8_t kSpsPpsExpected[] = {
  0x00, 0x00, 0x00, 0x16, 0x27, 0x42, 0xe0, 0x0d,
  0xa9, 0x18, 0x28, 0x3f, 0x60, 0x0d, 0x41, 0x80,
  0x41, 0xad, 0xb7, 0xa0, 0x2f, 0x01, 0xe9, 0x7b,
  0xdf, 0x01, 0x00, 0x00, 0x00, 0x04, 0x28, 0xce,
  0x09, 0x88};
const uint8_t kExpectedPssh[] = {
  0x00, 0x00, 0x00, 0x5c, 0x70, 0x73, 0x73, 0x68,
  0x00, 0x00, 0x00, 0x00, 0xed, 0xef, 0x8b, 0xa9,
  0x79, 0xd6, 0x4a, 0xce, 0xa3, 0xc8, 0x27, 0xdc,
  0xd5, 0x1d, 0x21, 0xed, 0x00, 0x00, 0x00, 0x3c,
  0x08, 0x01, 0x12, 0x10, 0xe5, 0x00, 0x7e, 0x6e,
  0x9d, 0xcd, 0x5a, 0xc0, 0x95, 0x20, 0x2e, 0xd3,
  0x75, 0x83, 0x82, 0xcd, 0x1a, 0x0d, 0x77, 0x69,
  0x64, 0x65, 0x76, 0x69, 0x6e, 0x65, 0x5f, 0x74,
  0x65, 0x73, 0x74, 0x22, 0x11, 0x54, 0x45, 0x53,
  0x54, 0x5f, 0x43, 0x4f, 0x4e, 0x54, 0x45, 0x4e,
  0x54, 0x5f, 0x49, 0x44, 0x5f, 0x31, 0x2a, 0x02,
  0x53, 0x44, 0x32, 0x00};

const size_t kDashHeaderRead = 10000;  // Enough to get the sidx.
uint32_t kPsshContext = 100;
uint32_t kDecryptionContext = 101;
}  // namespace

namespace dash2hls {
namespace internal {
DashToHlsStatus ProcessAvcc(const AvcCContents* avcc,
                            std::vector<uint8_t>* sps_pps);
}  // namespace internal

TEST(DashToHlsApi, ProcessAvcc) {
  DashParser parser;
  parser.Parse(kAvcCBox, sizeof(kAvcCBox));
  const Box* box = parser.FindDeep(BoxType::kBox_avcC);
  EXPECT_THAT(box, testing::NotNull());
  const AvcCContents *avcc =
      reinterpret_cast<const AvcCContents*>(box->get_contents());
  std::vector<uint8_t> sps_pps;
  EXPECT_EQ(kDashToHlsStatus_OK, internal::ProcessAvcc(avcc, &sps_pps));
  EXPECT_THAT(std::make_pair(&sps_pps[0], sps_pps.size()),
              testing::MemEq(kSpsPpsExpected, sizeof(kSpsPpsExpected)));
}

DashToHlsStatus PsshHandler(void* context, const uint8_t* pssh,
                            size_t pssh_length) {
  EXPECT_EQ(&kPsshContext, context);
  EXPECT_THAT(std::make_pair(pssh, pssh_length),
              testing::MemEq(kExpectedPssh, sizeof(kExpectedPssh)));
  return kDashToHlsStatus_OK;
}

// TODO(justsomeguy) There are test vectors from the widevine code that need
// to be ported.  The Decryption handler has to be tested before it can be
// used to test the actual product.
namespace {
const uint8_t video_key[] = {0x6f, 0xc9, 0x6f, 0xe6, 0x28, 0xa2, 0x65, 0xb1,
                             0x3a, 0xed, 0xde, 0xc0, 0xbc, 0x42, 0x1f, 0x4d};
}  // namespace

TEST(DashToHlsApi, ParseDash) {
  DashToHlsSession* session = nullptr;
  EXPECT_EQ(kDashToHlsStatus_OK, DashToHls_CreateSession(&session));
  FILE* file = Dash2HLS_GetTestVideoFile();
  ASSERT_NE(reinterpret_cast<FILE*>(0), file);
  size_t bytes_read = 0;
  uint8_t buffer[kDashHeaderRead];
  bytes_read = fread(buffer, 1, kDashHeaderRead, file);
  ASSERT_EQ(kDashHeaderRead, bytes_read);
  DashToHlsIndex* index;
  ASSERT_EQ(kDashToHlsStatus_ClearContent, DashToHls_ParseDash(session, buffer,
                                                     bytes_read, &index));
  const uint8_t* hls_segment;
  size_t hls_length;
  uint8_t* dash_buffer = new uint8_t[index->segments[0].length];
  fseek(file, index->segments[0].location, SEEK_SET);
  bytes_read = fread(dash_buffer, 1, index->segments[0].length, file);
  ASSERT_EQ(index->segments[0].length, bytes_read);
  size_t dash_buffer_size = index->segments[0].length;
  EXPECT_EQ(kDashToHlsStatus_OK,
            DashToHls_ConvertDashSegment(session, 0, dash_buffer,
                                         dash_buffer_size, &hls_segment,
                                         &hls_length));
  delete[] dash_buffer;
  fclose(file);
  // TODO(justsomeguy)  Currently the only way to test the output is to
  // write it to a file and open it with VLC.  Eventually we'll have known
  // good files and can compare.
  file = fopen("/tmp/out.ts", "w+");
  fwrite(hls_segment, 1, hls_length, file);
  fclose(file);

  EXPECT_EQ(kDashToHlsStatus_OK, DashToHls_ReleaseSession(session));
}

TEST(DashToHlsApi, ParseAudioDash) {
  uint32_t segment_number = 1;
  DashToHlsSession* session = nullptr;
  EXPECT_EQ(kDashToHlsStatus_OK, DashToHls_CreateSession(&session));
  FILE* file = Dash2HLS_GetTestAudioFile();
  ASSERT_NE(reinterpret_cast<FILE*>(0), file);
  size_t bytes_read = 0;
  uint8_t buffer[kDashHeaderRead];
  bytes_read = fread(buffer, 1, kDashHeaderRead, file);
  ASSERT_EQ(kDashHeaderRead, bytes_read);
  DashToHlsIndex* index;
  ASSERT_EQ(kDashToHlsStatus_ClearContent, DashToHls_ParseDash(session, buffer,
                                                     bytes_read, &index));
  const uint8_t* hls_segment;
  size_t hls_length;
  uint8_t* dash_buffer = new uint8_t[index->segments[segment_number].length];
  fseek(file, index->segments[segment_number].location, SEEK_SET);
  bytes_read = fread(dash_buffer, 1, index->segments[segment_number].length,
                     file);
  ASSERT_EQ(index->segments[segment_number].length, bytes_read);
  size_t dash_buffer_size = index->segments[segment_number].length;
  EXPECT_EQ(kDashToHlsStatus_OK,
            DashToHls_ConvertDashSegment(session, segment_number, dash_buffer,
                                         dash_buffer_size, &hls_segment,
                                         &hls_length));
  delete[] dash_buffer;
  fclose(file);
  // TODO(justsomeguy)  Currently the only way to test the output is to
  // write it to a file and open it with VLC.  Eventually we'll have known
  // good files and can compare.
  file = fopen("/tmp/out_audio.ts", "w+");
  fwrite(hls_segment, 1, hls_length, file);
  fclose(file);

  EXPECT_EQ(kDashToHlsStatus_OK, DashToHls_ReleaseSession(session));
}

TEST(DashToHlsApi, ParseDashList) {
  DashToHlsSession* session = nullptr;
  EXPECT_EQ(kDashToHlsStatus_OK, DashToHls_CreateSession(&session));
  FILE* file = Dash2HLS_GetTestVideoFile();
  ASSERT_NE(reinterpret_cast<FILE*>(0), file);
  size_t bytes_read = 0;
  uint8_t buffer[kDashHeaderRead];
  bytes_read = fread(buffer, 1, kDashHeaderRead, file);
  ASSERT_EQ(kDashHeaderRead, bytes_read);
  const uint8_t* hls_segment = nullptr;
  size_t hls_length = 0;

  ASSERT_EQ(kDashToHlsStatus_NeedMoreData,
            DashToHls_ParseLive(session, buffer, bytes_read, 0,
                                &hls_segment, &hls_length));
}

namespace internal {
  uint64_t GetDuration(const TrunContents* trun,
                       const TrunContents::TrackRun* track_run,
                       const TfhdContents* tfhd,
                       uint64_t trex_default_sample_duration);

}  // namespace internal

TEST(DashToHlsApi, DurationInTrex) {
  DashToHlsSession* session = nullptr;
  EXPECT_EQ(kDashToHlsStatus_OK, DashToHls_CreateSession(&session));
  FILE* file = Dash2HLS_GetMp4BoxInitSegment();
  ASSERT_NE(reinterpret_cast<FILE*>(0), file);
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  size_t bytes_read = 0;
  std::vector<uint8_t> buffer(file_size);
  bytes_read = fread(buffer.data(), 1, file_size, file);
  ASSERT_EQ(file_size, bytes_read);
  struct DashToHlsIndex* index = nullptr;
  DashToHlsStatus status = DashToHls_ParseDash(session, buffer.data(),
                                               bytes_read, &index);
  ASSERT_TRUE(status == kDashToHlsStatus_ClearContent ||
              status == kDashToHlsStatus_OK);
  Session* dash_session = reinterpret_cast<Session*>(session);
  std::vector<const Box*> boxes =
      dash_session->parser_.FindDeepAll(BoxType::kBox_tfhd);
  ASSERT_GE(1, boxes.size());
  const TfhdContents* tfhd =
      reinterpret_cast<const TfhdContents*>(boxes[0]->get_contents());

  boxes = dash_session->parser_.FindDeepAll(BoxType::kBox_trun);
  ASSERT_GE(1, boxes.size());
  const TrunContents* trun =
      reinterpret_cast<const TrunContents*>(boxes[0]->get_contents());

  boxes = dash_session->parser_.FindDeepAll(BoxType::kBox_trex);
  ASSERT_GE(1, boxes.size());
  const TrexContents* trex =
      reinterpret_cast<const TrexContents*>(boxes[0]->get_contents());

  // Duration is in the trex only so we can pass a nullptr for the particular
  // trun.
  EXPECT_EQ(1001, internal::GetDuration(trun, nullptr, tfhd,
                                        trex->get_default_sample_duration()));
  }
}  // namespace dash2hls
