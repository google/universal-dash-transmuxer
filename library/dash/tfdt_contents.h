#ifndef _DASH2HLS_TFDT_CONTENTS_H_
#define _DASH2HLS_TFDT_CONTENTS_H_

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

// The tfdt box contains the absolute media decode time.

#include <string>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class TfdtContents : public FullBoxContents {
 public:
  explicit TfdtContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_tfdt, stream_position) {}

  uint64_t get_base_media_decode_time() const {return base_media_decode_time_;}
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "TrackFragment";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint64_t base_media_decode_time_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_TFDT_CONTENTS_H_
