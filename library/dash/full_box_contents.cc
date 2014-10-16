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

#include "library/dash/full_box_contents.h"

#include <stdio.h>

#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace {
const uint32_t kBufferSize = 1024;
}  // namespace

namespace dash2hls {

// A FullBoxContents is the super class of most boxes with box specific
// data.  Each FullBoxContents has a version and flags.
size_t FullBoxContents::Parse(const uint8_t* buffer, size_t length) {
  if (!EnoughBytesToParse(0, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  version_ = static_cast<Version>(buffer[0]);
  if (version_ >= kVersionLast) {
    DASH_LOG((BoxName() + "  bad version").c_str(),
             "Only support version 0 and 1.",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  flags_ = (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
  return sizeof(flags_);
}

std::string FullBoxContents::PrettyPrint(std::string indent) const {
  char buffer[kBufferSize];
  snprintf(buffer, sizeof(buffer), "Version %d flags %x", version_, flags_);
  return buffer;
}
}  // namespace dash2hls
