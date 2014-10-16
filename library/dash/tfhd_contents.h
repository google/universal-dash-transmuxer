#ifndef _DASH2HLS_TFHD_CONTENTS_H_
#define _DASH2HLS_TFHD_CONTENTS_H_

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

// A TrackFragmentHeader sets default values for a run.  The trun box can
// specify these values on a sample by sample basis.  To save space a value
// set in this box can apply to every run.

#include <string>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class TfhdContents : public FullBoxContents {
 public:
  explicit TfhdContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_tfhd, stream_position) {}
  uint32_t get_default_sample_duration() const {
    return default_sample_duration_;
  }
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "TrackFragmentHeader";}

 protected:
  static const uint32_t kBaseDataOffsetPresentMask = 0x000001;
  static const uint32_t kSampleDecryptionIndexPresentMask = 0x000002;
  static const uint32_t kDefaultSampleDurationPresentMask = 0x000008;
  static const uint32_t kDefaultSampleSizePresentMask = 0x000010;
  static const uint32_t kDefaultSampleFlagsPresentMask = 0x000020;
  static const uint32_t kDurationIsEmptyMask = 0x010000;
  static const uint32_t kDefaultBaseIsMoofMask = 0x020000;

  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t track_id_;
  uint64_t base_data_offset_;
  uint32_t sample_description_index_;
  uint32_t default_sample_duration_;
  uint32_t default_sample_size_;
  uint32_t default_sample_flags_;  // TODO(justsomeguy) parse sample flags
};
}  // namespace dash2hls

#endif  // _DASH2HLS_TFHD_CONTENTS_H_
