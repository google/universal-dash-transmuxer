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

#include "library/dash/sample_entry_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14495-12 for details.
//
// aligned(8) abstract class SampleEntry (unsigned int(32) format)
// extends Box(format){
//   const unsigned int(8)[6] reserved = 0;
//   unsigned int(16) data_reference_index;
//
size_t SampleEntryContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* ptr = buffer;
  if (length < 6 + sizeof(uint16_t)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 6 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  ptr += sizeof(uint8_t) * 6;  // Reserved bytes to skip
  data_reference_index_ = ntohsFromBuffer(ptr);
  if (ptr + data_reference_index_ > buffer + length) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "Could not read data_reference_index_",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  ptr += sizeof(data_reference_index_);
  return ptr - buffer;
}

std::string SampleEntryContents::PrettyPrint(std::string indent) const {
  std::string result = " data_reference_index: ";
  result += PrettyPrintValue(data_reference_index_);
  result += BoxContents::PrettyPrint(indent);
  return result;
}
}  // namespace dash2hls
