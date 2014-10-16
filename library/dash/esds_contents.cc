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

#include "library/dash/esds_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-15 and 14495-12 for details.
//
// aligned(8) class ESDBox
// extends FullBox(‘esds’, version = 0, 0) {
//  ES_Descriptor ES;
// }
size_t EsdsContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  size_t bytes_parsed = descriptor_.Parse(ptr, length - (ptr - buffer));
  if (!bytes_parsed) {
    return DashParser::kParseFailure;
  }
  ptr += bytes_parsed;
  return ptr - buffer;
}

std::string EsdsContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += std::string("\n") + descriptor_.PrettyPrint(indent + "  ");
  return result;
}
}  // namespace dash2hls
