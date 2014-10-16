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

#include "library/dash/mp4a_contents.h"

#include "library/dash/box.h"
#include "library/dash/box_type.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-15 and 14495-12 for details.
//
// class MP4AudioSampleEntry() extends AudioSampleEntry ('mp4a'){
//  ESDBox ES;
// }
size_t Mp4aContents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* ptr = buffer +
      AudioSampleEntryContents::Parse(buffer, length);
  if (!dash_parser_) {
    DASH_LOG("Bad mp4a",
             "mp4a does not contain boxes inside it.",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  const Box* box = dash_parser_->Find(BoxType::kBox_esds);
  if (!box) {
    DASH_LOG("Bad mp4a",
             "mp4a does not contain an esds box.",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }

  esds_ = reinterpret_cast<const EsdsContents*>(box->get_contents());
  return ptr - buffer;
}

std::string Mp4aContents::PrettyPrint(std::string indent) const {
  std::string result = AudioSampleEntryContents::PrettyPrint(indent);
  return result;
}
}  // namespace dash2hls
