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

#include "library/dash/base_descriptor.h"

#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace {
// BaseDescriptors have a strange length scheme.  It's 7 bit length fields
// with a continue mask.
const uint8_t kContinueMask = 0x80;
const uint8_t kContinueShift = 7;
}  // namespace

namespace dash2hls {

size_t BaseDescriptor::Parse(const uint8_t* buffer, size_t length) {
  size_t result = 0;
  result += ParseHeader(buffer, length);
  if (result) {
    return result + size_;
  }
  return DashParser::kParseFailure;
}

size_t BaseDescriptor::ParseHeader(const uint8_t* buffer, size_t length) {
  if (!EnoughBytesToParse(0, sizeof(uint8_t), length)) {
    DASH_LOG("Descriptor too short", "At least 1 byte is required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }

  const uint8_t* bptr = buffer;

  tag_ = static_cast<Tag>(*bptr);
  ++bptr;
  while (true) {
    if (!EnoughBytesToParse(bptr-buffer, sizeof(uint8_t), length)) {
      DASH_LOG("Descriptor too short", "Length did not terminate",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    size_ <<= kContinueShift;
    size_ += (*bptr & ~kContinueMask);
    if (!(*bptr & kContinueMask)) {
      ++bptr;
      break;
    }
    ++bptr;
  }
  return bptr - buffer;
}

std::string BaseDescriptor::PrettyPrint(std::string indent) const {
  std::string result = indent + "Descriptor (";
  result += PrettyPrintValue(size_) + ")";
  switch (tag_) {
    case kForbiddenTag:
      result += " kForbiddenTag";
      break;
    case kObjectDescrTag:
      result += " kObjectDescrTag";
      break;
    case kInitialObjectDescrTag:
      result += " kInitialObjectDescrTag";
      break;
    case kES_DescrTag:
      result += " kES_DescrTag";
      break;
    case kDecoderConfigDescrTag:
      result += " kDecoderConfigDescrTag";
      break;
    case kDecSpecificInfoTag:
      result += " kDecSpecificInfoTag";
      break;
    case kSLConfigDescrTag:
      result += " kSLConfigDescrTag";
      break;
    case kContentIdentDescrTag:
      result += " kContentIdentDescrTag";
      break;
    case kSupplContentIdentDescrTag:
      result += " kSupplContentIdentDescrTag";
      break;
    case kIPI_DescrPointerTag:
      result += " kIPI_DescrPointerTag";
      break;
    case kIPMP_DescrPointerTag:
      result += " kIPMP_DescrPointerTag";
      break;
    case kIPMP_DescrTag:
      result += " kIPMP_DescrTag";
      break;
    case kQoS_DescrTag:
      result += " kQoS_DescrTag";
      break;
    case kRegistrationDescrTag:
      result += " kRegistrationDescrTag";
      break;
    case kES_ID_IncTag:
      result += " kES_ID_IncTag";
      break;
    case kES_ID_RefTag:
      result += " kES_ID_RefTag";
      break;
    case kMP4_IOD_Tag:
      result += " kMP4_IOD_Tag";
      break;
    case kMP4_OD_Tag:
      result += " kMP4_OD_Tag";
      break;
    case kIPL_DescrPointerRefTag:
      result += " kIPL_DescrPointerRefTag";
      break;
    case kExtensionProfileLevelDescrTag:
      result += " kExtensionProfileLevelDescrTag";
      break;
    case kprofileLevelIndicationIndexDescrTag:
      result += " kprofileLevelIndicationIndexDescrTag";
      break;
    case kContentClassificationDescrTag:
      result += " kContentClassificationDescrTag";
      break;
    case kKeyWordDescrTag:
      result += " kKeyWordDescrTag";
      break;
    case kRatingDescrTag:
      result += " kRatingDescrTag";
      break;
    case kLanguageDescrTag:
      result += " kLanguageDescrTag";
      break;
    case kShortTextualDescrTag:
      result += " kShortTextualDescrTag";
      break;
    case kExpandedTextualDescrTag:
      result += " kExpandedTextualDescrTag";
      break;
    case kContentCreatorNameDescrTag:
      result += " kContentCreatorNameDescrTag";
      break;
    case kContentCreationDateDescrTag:
      result += " kContentCreationDateDescrTag";
      break;
    case kOCICreatorNameDescrTag:
      result += " kOCICreatorNameDescrTag";
      break;
    case kOCICreationDateDescrTag:
      result += " kOCICreationDateDescrTag";
      break;
    case kSmpteCameraPositionDescrTag:
      result += " kSmpteCameraPositionDescrTag";
      break;
    case kSegmentDescrTag:
      result += " kSegmentDescrTag";
      break;
    case kMediaTimeDescrTag:
      result += " kMediaTimeDescrTag";
      break;
    case kIPMP_ToolsListDescrTag:
      result += " kIPMP_ToolsListDescrTag";
      break;
    case kIPMP_ToolTag:
      result += " kIPMP_ToolTag";
      break;
    case kM4MuxTimingDescrTag:
      result += " kM4MuxTimingDescrTag";
      break;
    case kM4MuxCodeTableDescrTag:
      result += " kM4MuxCodeTableDescrTag";
      break;
    case kExtSLConfigDescrTag:
      result += " kExtSLConfigDescrTag";
      break;
    case kM4MuxBufferSizeDescrTag:
      result += " kM4MuxBufferSizeDescrTag";
      break;
    case kM4MuxIdentDescrTag:
      result += " kM4MuxIdentDescrTag";
      break;
    case kDependencyPointerTag:
      result += " kDependencyPointerTag";
      break;
    case kDependencyMarkerTag:
      result += " kDependencyMarkerTag";
      break;
    case kM4MuxChannelDescrTag:
      result += " kM4MuxChannelDescrTag";
      break;
    case kForbiddenLastTag:
      result += " kForbiddenLastTag";
      break;
    default:
      result += " ReservedTag";
      break;
  }
  return result;
}
}  // namespace dash2hls
