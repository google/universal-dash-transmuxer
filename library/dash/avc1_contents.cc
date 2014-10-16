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

#include "library/dash/avc1_contents.h"

#include "library/dash/box.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-15 and 14495-12 for details.
//
// class AVCSampleEntry() extends VisualSampleEntry (‘avc1’){
//   AVCConfigurationBox config;
//   MPEG4BitRateBox (); // optional
//   MPEG4ExtensionDescriptorsBox (); // optional
// }
size_t Avc1Contents::Parse(const uint8_t *buffer, size_t length) {
  const uint8_t* ptr = buffer +
      VideoSampleEntryContents::Parse(buffer, length);
  return ptr - buffer;
}

std::string Avc1Contents::PrettyPrint(std::string indent) const {
  std::string result = VideoSampleEntryContents::PrettyPrint(indent);
  return result;
}
}  // namespace dash2hls
