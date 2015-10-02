#ifndef _DASH2HLS_MDHD_CONTENTS_H_
#define _DASH2HLS_MDHD_CONTENTS_H_

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

// The mvhd box contains information about the asset, the important one is
// the timescale.

#include <string>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class MdhdContents : public FullBoxContents {
 public:
  explicit MdhdContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_mvhd, stream_position) {}

  uint64_t get_creation_time() const {return creation_time_;}
  uint64_t get_modification_time() const {return modification_time_;}
  uint64_t get_timescale() const {return timescale_;}
  uint64_t get_duration() const {return duration_;}
  uint8_t get_language() const {return language_;}

  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "Movie Header";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint64_t creation_time_;
  uint64_t modification_time_;
  uint64_t timescale_;
  uint64_t duration_;
  uint8_t language_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_MDHD_CONTENTS_H_
