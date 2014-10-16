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

#include "utilities.h"

#include <gtest/gtest.h>

#include "include/DashToHlsApi.h"

namespace {
const uint8_t kUint8 = 0x01;
const uint16_t kUint16 = 0x0102;
const uint32_t kUint32 = 0x01020304;
const uint64_t kUint64 = 0x0102030405060708;
const int8_t kInt8 = -0x01;
const int16_t kInt16 = -0x0102;
const int32_t kInt32 = -0x01020304;
const int64_t kInt64 = -0x0102030405060708;
};

namespace dash2hls {

TEST(Dash2HLS, ntoh) {
  uint8_t buffer[sizeof(uint64_t)];
  for (size_t count = 0; count < sizeof(uint64_t); ++count) {
    buffer[count] = count + 1;
  }

  EXPECT_EQ(0x0102, ntohsFromBuffer(buffer));
  EXPECT_EQ(0x01020304U, ntohlFromBuffer(buffer));
  EXPECT_EQ(0x0102030405060708U, ntohllFromBuffer(buffer));
}

TEST(Dash2HLS, hton) {
  uint8_t buffer[sizeof(uint64_t)];
  htonsToBuffer(0x0102, buffer);
  htonlToBuffer(0x03040506, buffer + sizeof(uint16_t));
  for (size_t count = 0; count < sizeof(uint16_t) + sizeof(uint32_t);
       ++count) {
    EXPECT_EQ(count + 1, buffer[count]);
  }
  htonllToBuffer(kUint64, buffer);
  for (size_t count = 0; count < sizeof(uint64_t);
       ++count) {
    EXPECT_EQ(count + 1, buffer[count]);
  }
}

TEST(Dash2HLS, PrettyPrintValue) {
  EXPECT_EQ("1", PrettyPrintValue(kUint8));
  EXPECT_EQ("-1", PrettyPrintValue(kInt8));
  EXPECT_EQ("258", PrettyPrintValue(kUint16));
  EXPECT_EQ("-258", PrettyPrintValue(kInt16));
  EXPECT_EQ("16909060", PrettyPrintValue(kUint32));
  EXPECT_EQ("-16909060", PrettyPrintValue(kInt32));
  EXPECT_EQ("72623859790382856", PrettyPrintValue(kUint64));
  EXPECT_EQ("-72623859790382856", PrettyPrintValue(kInt64));
  size_t a_size = kUint16;
  EXPECT_EQ("258", PrettyPrintValue(a_size));
}

TEST(Dash2HLS, EnoughBytesToParse) {
  EXPECT_TRUE(EnoughBytesToParse(0, 10, 10));
  EXPECT_TRUE(EnoughBytesToParse(5, 5, 15));
  EXPECT_TRUE(EnoughBytesToParse(10, 0, 10));
  EXPECT_FALSE(EnoughBytesToParse(0, 10, 5));
  EXPECT_FALSE(EnoughBytesToParse(10, 10, 15));
  EXPECT_FALSE(EnoughBytesToParse(10, 10, 19));
}

namespace {
std::string s_callback_result;
void s_callback(const char* message) {
  s_callback_result = message;
}
}  // namespace

TEST(Dash2Hls, DiagnosticCallback) {
  SetDiagnosticCallback(s_callback);
  char line_buffer[1024];

  // The sprintf must be immediatedly after the DASH_LOG.
  DASH_LOG("message", "reason", nullptr);
  snprintf(line_buffer, sizeof(line_buffer), "%u", __LINE__ -1);
  std::string expected = "{\n\t'Message':'message',\n\t'File':'";
  expected += std::string(__FILE__) + "',\n\t'Line':" + line_buffer;
  expected += std::string(",\n\t'Reason':'reason'\n}");
  EXPECT_EQ(expected, s_callback_result);

  // The sprintf must be immediatedly after the DASH_LOG.
  DASH_LOG("message", "reason", "extra_fields");
  snprintf(line_buffer, sizeof(line_buffer), "%u", __LINE__ - 1);
  expected = "{\n\t'Message':'message',\n\t'File':'";
  expected += std::string(__FILE__) + "',\n\t'Line':" + line_buffer +
      std::string(",\n\t'Reason':'reason',\n\t'Extra':'extra_fields'\n}");
  EXPECT_EQ(expected, s_callback_result);
  SetDiagnosticCallback(nullptr);
}
}  // namespace dash2hls
