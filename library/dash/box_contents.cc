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

#include "library/dash/box_contents.h"
#include "library/dash/dash_parser.h"

namespace dash2hls {
BoxContents::BoxContents(uint32_t box_type, uint64_t stream_position)
    : stream_position_(stream_position),
      dash_parser_(nullptr),
      box_type_(box_type) {
}

BoxContents::~BoxContents() {
  delete dash_parser_;
}

size_t BoxContents::Parse(const uint8_t* buffer, size_t length) {
  if (!dash_parser_) {
    dash_parser_ = new DashParser;
    dash_parser_->set_current_position(stream_position_);
  }
  return dash_parser_->Parse(buffer, length);
}

std::string BoxContents::PrettyPrint(std::string indent) const {
  if (dash_parser_) {
    return "\n" + dash_parser_->PrettyPrint(indent);
  } else {
    return "";
  }
}

// BoxContents can have their own BoxName.  These are the common boxes that
// are not subclassed.
std::string BoxContents::BoxName() const {
  switch (box_type_) {
    case BoxType::kBox_dinf: return "DataInformation";
    case BoxType::kBox_edts: return "Edit (time mapping)";
    case BoxType::kBox_mdat: return "MediaData";
    case BoxType::kBox_mdia: return "Media";
    case BoxType::kBox_minf: return "MediaInformation";
    case BoxType::kBox_moof: return "MovieFragment";
    case BoxType::kBox_moov: return "Movie";
    case BoxType::kBox_mvex: return "MovieExtend";
    case BoxType::kBox_stbl: return "SampleTable";
    case BoxType::kBox_traf: return "TrackFragment";
    case BoxType::kBox_trak: return "Track";
    default: return "Unknown";
  }
}
}  // namespace dash2hls
