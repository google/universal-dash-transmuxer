#ifndef _DASH2HLS_PRFT_CONTENTS_H_
#define _DASH2HLS_PRFT_CONTENTS_H_

/*
 Copyright 2015 Google Inc. All rights reserved.

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

#include <string>
#include <vector>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

// prft = Producer reference time box. Maps media time on the media clock to a
// real world wall time at which the media was produced.
class PrftContents : public FullBoxContents {
 public:
  explicit PrftContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_prft, stream_position) {}

  const int32_t get_reference_track_id() const { return reference_track_id_; }
  const int64_t get_ntp_timestamp() const { return ntp_timestamp_; }
  const int64_t get_media_time() const { return media_time_; }

  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const { return "ProducerReferenceTime"; }

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  int32_t reference_track_id_;
  int64_t ntp_timestamp_;
  int64_t media_time_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_PRFT_CONTENTS_H_
