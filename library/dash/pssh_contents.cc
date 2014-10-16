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

#include "library/dash/pssh_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 23001-7 for details.
// aligned(8) class ProtectionSystemSpecificHeaderBox extends FullBox(‘pssh’,
// version=0, flags=0)
// {
//   unsigned int(8)[16] SystemID;
//   unsigned int(32) DataSize;
//   unsigned int(8)[DataSize] Data;
// }
size_t PsshContents::Parse(const uint8_t *buffer, size_t length) {
  full_box_.resize(length + sizeof(uint32_t) * 2);
  htonlToBuffer(static_cast<uint32_t>(length + sizeof(uint32_t) * 2),
                &full_box_[0]);
  full_box_[4] = 'p';
  full_box_[5] = 's';
  full_box_[6] = 's';
  full_box_[7] = 'h';
  memcpy(&full_box_[sizeof(uint32_t) * 2], buffer, length);
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  contents_.resize(length - (ptr - buffer));
  memcpy(&contents_[0], ptr, contents_.size());
  if (!EnoughBytesToParse(ptr - buffer, sizeof(system_id_) + sizeof(uint32_t),
                          length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 20 more bytes is required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  memcpy(system_id_, ptr, sizeof(system_id_));
  ptr += sizeof(system_id_);
  data_.resize(ntohlFromBuffer(ptr));
  ptr += sizeof(uint32_t);
  if (!EnoughBytesToParse(ptr - buffer, data_.size(), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "Data missing.",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  memcpy(&data_[0], ptr, data_.size());
  ptr += data_.size();
  return ptr - buffer;
}

std::string PsshContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += "SystemId: " + PrettyPrintBuffer(system_id_, sizeof(system_id_));
  if (g_verbose_pretty_print) {
    result += "Data: " + PrettyPrintBuffer(data_.data(), data_.size());
  }
  return result;
}
}  //  namespace dash2hls
