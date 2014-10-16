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

#include "library/dash/mdat_contents.h"

#include <string>

#include "library/utilities.h"

namespace dash2hls {
// See ISO 14495-12 for details.
//
// Mdat is the raw samples concatenated without any markers or framing.
// Other boxes are required to find the starts and stops of samples.
size_t MdatContents::Parse(const uint8_t* buffer, size_t length) {
  raw_data_ = buffer;
  raw_data_length_ = length;
  return length;
}

std::string MdatContents::PrettyPrint(std::string indent) const {
  std::string result = BoxContents::PrettyPrint(indent);
  result += " " + PrettyPrintValue(raw_data_length_) + " bytes";
  return result;
}
}  // namespace dash2hls
