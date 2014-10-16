#ifndef _DASH2HLS_VIDEO_SAMPLE_ENTRY_CONTENTS_H_
#define _DASH2HLS_VIDEO_SAMPLE_ENTRY_CONTENTS_H_

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

// Sample box containing the codec information.

#include <string>

#include "library/dash/sample_entry_contents.h"

namespace dash2hls {

class VideoSampleEntryContents : public SampleEntryContents {
 public:
  VideoSampleEntryContents(uint32_t type, uint64_t stream_position)
      : SampleEntryContents(type, stream_position) {}
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "SampleEntry";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  enum {
    kCompressorLength = 32,
  };
  uint16_t width_;
  uint16_t height_;
  uint16_t horiz_resolution_[2];
  uint16_t vert_resolution_[2];
  uint16_t frame_count_;
  char compressor_name_[kCompressorLength];
  uint16_t depth_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_VIDEO_SAMPLE_CONTENTS_H_
