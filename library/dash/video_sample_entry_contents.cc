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

#include "library/dash/video_sample_entry_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace {
const size_t kMinBoxSize = 70;
}  // namespace

namespace dash2hls {

// See ISO 14496-15 and 14495-12 for details.
//
// class VisualSampleEntry(codingname) extends SampleEntry (codingname) {
//   unsigned int(16) pre_defined = 0;
//   const unsigned int(16) reserved = 0;
//   unsigned int(32)[3] pre_defined = 0;
//   unsigned int(16) width;
//   unsigned int(16) height;
//   template unsigned int(32) horizresolution = 0x00480000; // 72 dpi
//   template unsigned int(32) vertresolution = 0x00480000; // 72 dpi
//   const unsigned int(32) reserved = 0;
//   template unsigned int(16) frame_count = 1;
//   string[32] compressorname;
//   template unsigned int(16) depth = 0x0018;
//   int(16) pre_defined = -1;
//   // other boxes from derived specifications
//   CleanApertureBox clap; // optional
//   PixelAspectRatioBox pasp; // optional
// }
size_t VideoSampleEntryContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* volatile ptr =
      buffer + SampleEntryContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, kMinBoxSize, length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 70 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  // Reserved bytes to skip
  ptr += sizeof(uint16_t) * 2 + sizeof(uint32_t) * 3;
  width_ = ntohsFromBuffer(ptr);
  ptr += sizeof(width_);
  height_ = ntohsFromBuffer(ptr);
  ptr += sizeof(height_);
  horiz_resolution_[0] = ntohsFromBuffer(ptr);
  ptr += sizeof(horiz_resolution_[0]);
  horiz_resolution_[1] = ntohsFromBuffer(ptr);
  ptr += sizeof(horiz_resolution_[1]);
  vert_resolution_[0] = ntohsFromBuffer(ptr);
  ptr += sizeof(vert_resolution_[0]);
  vert_resolution_[1] = ntohsFromBuffer(ptr);
  ptr += sizeof(vert_resolution_[1]);
  ptr += sizeof(uint32_t);  // Reserved.
  frame_count_ = ntohsFromBuffer(ptr);
  ptr += sizeof(frame_count_);
  size_t compressor_size = ptr[0];
  if (compressor_size > kCompressorLength - 1) {
      compressor_size = kCompressorLength - 1;
  }
  memcpy(compressor_name_, ptr + 1, compressor_size);
  compressor_name_[compressor_size] = 0;
  ptr += kCompressorLength;
  depth_ = ntohsFromBuffer(ptr);
  ptr += sizeof(depth_);
  ptr += sizeof(uint16_t);  // pre_defined.

  // done parsing mandatory fields, see if there's optional boxes.
  size_t bytes_left = length - (ptr - buffer);
  if (bytes_left > 0) {
    delete dash_parser_;
    dash_parser_ = new DashParser;
    dash_parser_->set_current_position(stream_position_ + bytes_left);
    size_t bytes_parsed = dash_parser_->Parse(ptr, bytes_left);
    if (bytes_parsed != bytes_left) {
      return DashParser::kParseFailure;
    }
    ptr += bytes_parsed;
  }
  return ptr - buffer;
}

std::string VideoSampleEntryContents::PrettyPrint(std::string indent) const {
  std::string result = " width: " + PrettyPrintValue(width_);
  result += " height: " + PrettyPrintValue(height_);
  result += " horiz: " + PrettyPrintValue(horiz_resolution_[0]) + "." +
      PrettyPrintValue(horiz_resolution_[1]);
  result += " vert: " + PrettyPrintValue(vert_resolution_[0]) + "." +
      PrettyPrintValue(vert_resolution_[1]);
  result += " frame: " + PrettyPrintValue(frame_count_);
  result += " compressor: " + std::string(compressor_name_);
  result += " depth: " + PrettyPrintValue(depth_);
  if (dash_parser_) {
    result += "\n" + dash_parser_->PrettyPrint(indent + "  ");
  }
  result += SampleEntryContents::PrettyPrint(indent);
  return result;
}
}  // namespace dash2hls
