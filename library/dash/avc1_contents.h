#ifndef _DASH2HLS_AVC1_CONTENTS_H_
#define _DASH2HLS_AVC1_CONTENTS_H_

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

#include "library/dash/box_type.h"
#include "library/dash/video_sample_entry_contents.h"

namespace dash2hls {

class Avc1Contents : public VideoSampleEntryContents {
 public:
  explicit Avc1Contents(uint64_t stream_position)
      : VideoSampleEntryContents(BoxType::kBox_avc1, stream_position) {}
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "AVC1 Decode Information";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
};
}  // namespace dash2hls

#endif  // _DASH2HLS_AVC1_CONTENTS_H_
