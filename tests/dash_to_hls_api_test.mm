/*
 Copyright 2015 Google Inc. All rights reserved.

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

#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#include "include/DashToHlsApi.h"
#include "library/dash/avcc_contents.h"
#include "library/dash/box_type.h"
#include "library/dash/cbmp_contents.h"
#include "library/dash/dash_parser.h"
#include "library/dash/emsg_contents.h"
#include "library/dash/equi_contents.h"
#include "library/dash/mdat_contents.h"
#include "library/dash/mshp_contents.h"
#include "library/dash/prft_contents.h"
#include "library/dash/prhd_contents.h"
#include "library/dash/saio_contents.h"
#include "library/dash/saiz_contents.h"
#include "library/dash/st3d_contents.h"
#include "library/dash/tenc_contents.h"
#include "library/dash/tfdt_contents.h"
#include "library/dash/tfhd_contents.h"
#include "library/dash/trex_contents.h"
#include "library/dash/trun_contents.h"
#include "library/dash_to_hls_session.h"
#include "library/utilities.h"
#include "tests/mac_test_files.h"

const uint8_t kAvcCBox[] = {0x00, 0x00, 0x00, 0x2d,
    0x61, 0x76, 0x63, 0x43, 0x01, 0x42, 0xe0, 0x0d,
    0xff, 0xe1, 0x00, 0x16, 0x27, 0x42, 0xe0, 0x0d,
    0xa9, 0x18, 0x28, 0x3f, 0x60, 0x0d, 0x41, 0x80,
    0x41, 0xad, 0xb7, 0xa0, 0x2f, 0x01, 0xe9, 0x7b,
    0xdf, 0x01, 0x01, 0x00, 0x04, 0x28, 0xce, 0x09,
    0x88};
uint8_t kSpsPpsExpected[] = {
    0x00, 0x00, 0x00, 0x16, 0x27, 0x42, 0xe0, 0x0d,
    0xa9, 0x18, 0x28, 0x3f, 0x60, 0x0d, 0x41, 0x80,
    0x41, 0xad, 0xb7, 0xa0, 0x2f, 0x01, 0xe9, 0x7b,
    0xdf, 0x01, 0x00, 0x00, 0x00, 0x04, 0x28, 0xce,
    0x09, 0x88};
/*  TODO(seawardt@): Investigate comparing of PSSH.
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
*/
const uint8_t kPrftBox[] = {
    0x00, 0x00, 0x00, 0x20, 0x70, 0x72, 0x66, 0x74,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff};
const uint8_t kSt3dBox[] = {
    0x00, 0x00, 0x00, 0x0d, 0x73, 0x74, 0x33, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x02};
const uint8_t kPrhdBox[] = {
    0x00, 0x00, 0x00, 0x18, 0x70, 0x72, 0x68, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03};
const uint8_t kCbmpBox[] = {
    0x00, 0x00, 0x00, 0x14, 0x63, 0x62, 0x6d, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01};
const uint8_t kEquiBox[] = {
    0x00, 0x00, 0x00, 0x1c, 0x65, 0x71, 0x75, 0x69,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x04};
const uint8_t kMshpBox[] = {
    0x00, 0x00, 0x00, 0x15, 0x6d, 0x73, 0x68, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x90, 0x8e, 0xa3,
    0x72, 0x61, 0x77, 0x20, 0x00};

const size_t kDashHeaderRead = 10000;  // Enough to get the sidx.
uint32_t kPsshContext = 100;
uint32_t kDecryptionContext = 101;
// TODO(seawardt) There are test vectors from the widevine code that need
// to be ported.  The Decryption handler has to be tested before it can be
// used to test the actual product.
// const uint8_t video_key[] = {0x6f, 0xc9, 0x6f, 0xe6, 0x28, 0xa2, 0x65, 0xb1, 0x3a, 0xed, 0xde,
// 0xc0, 0xbc, 0x42, 0x1f, 0x4d};

@interface UdtTests : XCTestCase
@end


namespace dash2hls {
namespace internal {
DashToHlsStatus ProcessAvcc(const dash2hls::AvcCContents* avcc,
                            uint8_t stream_number,
                            std::vector<uint8_t>* sps_pps);
} // namespace internal
} // namespace dash2hls

extern "C" void DashToHls_TestContent(unsigned char* content, size_t length);
uint64_t GetDuration(const dash2hls::TrunContents* trun,
                     const dash2hls::TrunContents::TrackRun* track_run,
                     const dash2hls::TfhdContents* tfhd,
                     uint64_t trex_default_sample_duration);

@implementation UdtTests


- (void)setUp {
  [super setUp];
}

- (void)testProcessAvcc {
  dash2hls::DashParser parser;
  parser.Parse(kAvcCBox, sizeof(kAvcCBox));
  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_avcC);
  XCTAssertNotEqual(box, nullptr);
  const dash2hls::AvcCContents *avcc =
      reinterpret_cast<const dash2hls::AvcCContents*>(box->get_contents());
  std::vector<uint8_t> sps_pps;
  XCTAssertEqual(kDashToHlsStatus_OK, dash2hls::internal::ProcessAvcc(avcc, 0, &sps_pps));
  XCTAssertEqual(sps_pps.size(), sizeof(kSpsPpsExpected));
  XCTAssertEqual([NSString stringWithUTF8String:(char*)kSpsPpsExpected],
                 [NSString stringWithUTF8String:(char*)&sps_pps[0]]);
}

- (void)testProcessAvccSize {
  dash2hls::DashParser parser;
  parser.Parse(kAvcCBox, sizeof(kAvcCBox));
  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_avcC);
  XCTAssertNotEqual(box, nullptr);
  const dash2hls::AvcCContents *avcc =
      reinterpret_cast<const dash2hls::AvcCContents*>(box->get_contents());
  std::vector<uint8_t> sps_pps;
  XCTAssertEqual(kDashToHlsStatus_OK, dash2hls::internal::ProcessAvcc(avcc, 2, &sps_pps));
  XCTAssertEqual(sps_pps.size(), sizeof(kSpsPpsExpected));
  XCTAssertEqual([NSString stringWithUTF8String:(char*)kSpsPpsExpected],
                 [NSString stringWithUTF8String:(char*)&sps_pps[0]]);
}

- (void)testParseDash {
  DashToHlsSession* session = nullptr;
  XCTAssertEqual(kDashToHlsStatus_OK, DashToHls_CreateSession(&session));
  FILE* file = Dash2HLS_GetTestVideoFile();
  XCTAssertNotEqual(reinterpret_cast<FILE*>(0), file);
  size_t bytes_read = 0;
  uint8_t buffer[kDashHeaderRead];
  bytes_read = fread(buffer, 1, kDashHeaderRead, file);
  XCTAssertEqual(kDashHeaderRead, bytes_read);
  DashToHlsIndex* index;
  XCTAssertEqual(kDashToHlsStatus_ClearContent, DashToHls_ParseDash(session, buffer,
                                                               bytes_read, &index));
  const uint8_t* hls_segment;
  size_t hls_length;
  uint8_t* dash_buffer = new uint8_t[index->segments[0].length];
  fseek(file, index->segments[0].location, SEEK_SET);
  bytes_read = fread(dash_buffer, 1, index->segments[0].length, file);
  XCTAssertEqual(index->segments[0].length, bytes_read);
  size_t dash_buffer_size = index->segments[0].length;
  XCTAssertEqual(kDashToHlsStatus_OK,
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
  XCTAssertEqual(kDashToHlsStatus_OK, DashToHls_ReleaseSession(session));
}

- (void)testParseAudioDash {
  uint32_t segment_number = 1;
  DashToHlsSession* session = nullptr;
  XCTAssertEqual(kDashToHlsStatus_OK, DashToHls_CreateSession(&session));
  FILE* file = Dash2HLS_GetTestAudioFile();
  XCTAssertNotEqual(reinterpret_cast<FILE*>(0), file);
  size_t bytes_read = 0;
  uint8_t buffer[kDashHeaderRead];
  bytes_read = fread(buffer, 1, kDashHeaderRead, file);
  XCTAssertEqual(kDashHeaderRead, bytes_read);
  DashToHlsIndex* index;
  XCTAssertEqual(kDashToHlsStatus_ClearContent, DashToHls_ParseDash(session, buffer,
                                                               bytes_read, &index));
  const uint8_t* hls_segment;
  size_t hls_length;
  uint8_t* dash_buffer = new uint8_t[index->segments[segment_number].length];
  fseek(file, index->segments[segment_number].location, SEEK_SET);
  bytes_read = fread(dash_buffer, 1, index->segments[segment_number].length,
                     file);
  XCTAssertEqual(index->segments[segment_number].length, bytes_read);
  size_t dash_buffer_size = index->segments[segment_number].length;
  XCTAssertEqual(kDashToHlsStatus_OK,
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

  XCTAssertEqual(kDashToHlsStatus_OK, DashToHls_ReleaseSession(session));
}

- (void)testParseDashList {
  DashToHlsSession* session = nullptr;
  XCTAssertEqual(kDashToHlsStatus_OK, DashToHls_CreateSession(&session));
  FILE* file = Dash2HLS_GetTestVideoFile();
  XCTAssertNotEqual(reinterpret_cast<FILE*>(0), file);
  size_t bytes_read = 0;
  uint8_t buffer[kDashHeaderRead];
  bytes_read = fread(buffer, 1, kDashHeaderRead, file);
  XCTAssertEqual(kDashHeaderRead, bytes_read);
  const uint8_t* hls_segment = nullptr;
  size_t hls_length = 0;

  XCTAssertEqual(kDashToHlsStatus_NeedMoreData,
            DashToHls_ParseLive(session, buffer, bytes_read, 0,
                                &hls_segment, &hls_length));
}

- (void)testTheTestRoutine {
  FILE* file = Dash2HLS_GetTestVideoFile();
  XCTAssertNotEqual(reinterpret_cast<FILE*>(0), file);
  size_t bytes_read = 0;
  fseek(file, 0, SEEK_END);
  size_t bytes_to_read = ftell(file);
  fseek(file, 0, SEEK_SET);
  std::vector<uint8_t> buffer(bytes_to_read);

  bytes_read = fread(buffer.data(), 1, bytes_to_read, file);
  XCTAssertEqual(bytes_to_read, bytes_read);
  DashToHls_TestContent(buffer.data(), buffer.size());
}

- (void)testParseEmsg {
  NSData* emsgBoxData = [[NSFileManager defaultManager]
      contentsAtPath:Dash2HLS_GetMp4BoxEmsgPath()];

  dash2hls::DashParser parser;
  parser.Parse((const uint8_t*)emsgBoxData.bytes, emsgBoxData.length);

  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_emsg);
  XCTAssertNotEqual(box, nullptr);

  const dash2hls::EmsgContents* emsg =
      reinterpret_cast<const dash2hls::EmsgContents*>(box->get_contents());

  XCTAssertEqualObjects(
      @"scheme_id_uri", [NSString stringWithUTF8String:emsg->get_scheme_id_uri().c_str()]);
  XCTAssertEqualObjects(@"value", [NSString stringWithUTF8String:emsg->get_value().c_str()]);
  XCTAssertEqual(90000, emsg->get_timescale());
  XCTAssertEqual(3600 * 90000, emsg->get_presentation_time_delta());
  XCTAssertEqual(10 * 90000, emsg->get_event_duration());
  XCTAssertEqual(9001, emsg->get_id());
  XCTAssertEqualObjects(
      @"message_data", [[NSString alloc] initWithBytes:&emsg->get_message_data().front()
                                                length:emsg->get_message_data().size()
                                              encoding:NSASCIIStringEncoding]);

  NSString* expectedPrettyPrint =
      @"Version 0 flags 0\n"
       "  scheme_id_uri: scheme_id_uri\n"
       "  value: value\n"
       "  timescale: 90000\n"
       "  presentation_time_delta: 324000000\n"
       "  event_duration: 900000\n"
       "  id: 9001\n"
       "  message_data:  0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x5f\n"
       " 0x64, 0x61, 0x74, 0x61";
  XCTAssertEqualObjects(
      expectedPrettyPrint,
      [NSString stringWithUTF8String:emsg->PrettyPrint("  ").c_str()]);
}

- (void)testParsePrft {
  dash2hls::DashParser parser;
  parser.Parse(kPrftBox, sizeof(kPrftBox));

  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_prft);
  XCTAssertNotEqual(box, nullptr);

  const dash2hls::PrftContents* prft =
    reinterpret_cast<const dash2hls::PrftContents*>(box->get_contents());

  XCTAssertEqual(15, prft->get_reference_track_id());
  XCTAssertEqual(0xff00000000000000, prft->get_ntp_timestamp());
  XCTAssertEqual(4095, prft->get_media_time());

  NSString* expectedPrettyPrint =
      @"Version 1 flags 0\n"
       "  reference_track_id: 15\n"
       "  ntp_timestamp: -72057594037927936\n"
       "  media_time: 4095\n";
  XCTAssertEqualObjects(
      expectedPrettyPrint,
      [NSString stringWithUTF8String:prft->PrettyPrint("  ").c_str()]);
}

- (void)testParseSt3d {
  dash2hls::DashParser parser;
  parser.Parse(kSt3dBox, sizeof(kSt3dBox));

  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_st3d);
  XCTAssertNotEqual(box, nullptr);

  const dash2hls::St3dContents* st3d =
      static_cast<const dash2hls::St3dContents*>(box->get_contents());

  XCTAssertEqual(2, st3d->get_stereo_mode());

  NSString* expectedPrettyPrint =
      @"Version 0 flags 0 Stereo Mode: 2";
  XCTAssertEqualObjects(expectedPrettyPrint,
                        [NSString stringWithUTF8String:st3d->PrettyPrint("  ").c_str()]);
}

- (void)testParsePrhd {
  dash2hls::DashParser parser;
  parser.Parse(kPrhdBox, sizeof(kPrhdBox));

  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_prhd);
  XCTAssertNotEqual(box, nullptr);

  const dash2hls::PrhdContents* prhd =
      static_cast<const dash2hls::PrhdContents*>(box->get_contents());

  XCTAssertEqual(1 / 0x1p16, prhd->get_pose_yaw_degrees());
  XCTAssertEqual(2 / 0x1p16, prhd->get_pose_pitch_degrees());
  XCTAssertEqual(3 / 0x1p16, prhd->get_pose_roll_degrees());
}

- (void)testParseCbmp {
  dash2hls::DashParser parser;
  parser.Parse(kCbmpBox, sizeof(kCbmpBox));

  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_cbmp);
  XCTAssertNotEqual(box, nullptr);

  const dash2hls::CbmpContents* cbmp =
      static_cast<const dash2hls::CbmpContents*>(box->get_contents());

  XCTAssertEqual(0, cbmp->get_layout());
  XCTAssertEqual(1, cbmp->get_padding());

  NSString* expectedPrettyPrint =
      @"Version 0 flags 0 Layout: 0 Padding: 1";
  XCTAssertEqualObjects(expectedPrettyPrint,
                        [NSString stringWithUTF8String:cbmp->PrettyPrint("  ").c_str()]);
}

- (void)testParseEqui {
  dash2hls::DashParser parser;
  parser.Parse(kEquiBox, sizeof(kEquiBox));

  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_equi);
  XCTAssertNotEqual(box, nullptr);

  const dash2hls::EquiContents* equi =
      static_cast<const dash2hls::EquiContents*>(box->get_contents());

  XCTAssertEqual(1 / 0x1p32, equi->get_projection_bounds_top());
  XCTAssertEqual(2 / 0x1p32, equi->get_projection_bounds_bottom());
  XCTAssertEqual(3 / 0x1p32, equi->get_projection_bounds_left());
  XCTAssertEqual(4 / 0x1p32, equi->get_projection_bounds_right());
}

- (void)testParseMshp {
  dash2hls::DashParser parser;
  parser.Parse(kMshpBox, sizeof(kMshpBox));

  const dash2hls::Box* box = parser.FindDeep(dash2hls::BoxType::kBox_mshp);
  XCTAssertNotEqual(box, nullptr);

  const dash2hls::MshpContents* mshp =
      static_cast<const dash2hls::MshpContents*>(box->get_contents());

  XCTAssertEqual(0x02908ea3, mshp->get_crc32());
  XCTAssertEqual('raw ', mshp->get_encoding_four_cc());
  XCTAssertNotEqual(nullptr, mshp->get_raw_data());
  XCTAssertEqual(1, mshp->get_raw_data_length());

  NSString* expectedPrettyPrint =
      @"Version 0 flags 0 CRC-32: 43028131 Encoding: 1918990112 1 bytes";
  XCTAssertEqualObjects(expectedPrettyPrint,
                        [NSString stringWithUTF8String:mshp->PrettyPrint("  ").c_str()]);
}

@end
