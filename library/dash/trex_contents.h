#ifndef _DASH2HLS_TREX_CONTENTS_H_
#define _DASH2HLS_TREX_CONTENTS_H_

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

// A TrackExtends sets default values for all movie fragments.

#include <string>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class TrexContents : public FullBoxContents {
 public:
  explicit TrexContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_trex, stream_position) {}

  uint32_t get_track_id() const {
    return track_id_;
  }
  uint32_t get_default_sample_description_index() const {
    return default_sample_description_index_;
  }
  uint32_t get_default_sample_duration() const {
    return default_sample_duration_;
  }
  uint32_t get_default_sample_size() const {
    return default_sample_size_;
  }
  uint32_t get_default_sample_flags() const {
    return default_sample_flags_;
  }

  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {
    return "TrackExtends";
  }

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t track_id_;
  uint32_t default_sample_description_index_;
  uint32_t default_sample_duration_;
  uint32_t default_sample_size_;
  uint32_t default_sample_flags_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_TREX_CONTENTS_H_
