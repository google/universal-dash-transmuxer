#ifndef _DASH2HLS_SIDX_CONTENTS_H_
#define _DASH2HLS_SIDX_CONTENTS_H_

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

// A sidx contains the offset and size to find each moof/mdat set of boxes.

#include <string>
#include <vector>

#include "include/DashToHlsApi.h"
#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class SidxContents : public FullBoxContents {
 public:
  explicit SidxContents(uint64_t stream_position) :
      FullBoxContents(BoxType::kBox_sidx, stream_position) {}
  // Each Reference is one segment of data.
  class Reference {
   public:
    bool segment_index_type_;
    uint32_t size_;
    uint32_t subsegment_duration_;
    bool starts_with_sap_;
    uint8_t sap_type_;
    uint32_t sap_delta_time;
  };
  const std::vector<DashToHlsSegment>& get_locations() const {
    return locations_;
  }
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "SegmentIndex";}
  uint32_t get_timescale() const {return timescale_;}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t reference_id_;
  uint32_t timescale_;
  uint64_t earliest_presentation_time_;
  uint64_t first_offset_;
  uint16_t reference_count_;
  std::vector<Reference> references_;
  std::vector<DashToHlsSegment> locations_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_SIDX_CONTENTS_H_
