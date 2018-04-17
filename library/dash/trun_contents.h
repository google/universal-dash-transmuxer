#ifndef _DASH2HLS_TRUN_CONTENTS_H_
#define _DASH2HLS_TRUN_CONTENTS_H_

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

// TrackRun boxes contain the offsets to each sample in an mdat.
//
// Each field is optional but all runs either have the field or don't.

#include <string>
#include <vector>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class TrunContents : public FullBoxContents {
 public:
  // Each value is optional and unitialized if not present.
  class TrackRun {
   public:
    uint32_t sample_duration_;
    uint32_t sample_size_;
    uint32_t sample_flags_;
    int64_t sample_composition_time_offset_;
  };

  explicit TrunContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_trex, stream_position) {}

  // Fields are optional.
  bool IsBaseDataOffsetPresent() const;
  bool IsFirstSampleFlagsPresent() const;
  bool IsSampleDurationPresent() const;
  bool IsSampleSizePresent() const;
  bool IsSampleFlagsPresent() const;
  bool IsSampleCompositionPresent() const;

  const std::vector<TrackRun>& get_track_runs() const {return track_runs_;}
  int32_t get_data_offset() const {return data_offset_;}

  virtual std::string PrettyPrint(std::string indent) const;
  std::string PrettyPrintTrackRun(const TrackRun& run) const;
  virtual std::string BoxName() const {return "TrackFragmentRun";}

 protected:
  static const uint32_t kBaseDataOffsetPresentMask = 0x000001;
  static const uint32_t kFirstSampleFlagsPresentMask = 0x000004;
  static const uint32_t kSampleDurationPresentMask = 0x0000100;
  static const uint32_t kSampleSizePresentMask = 0x000200;
  static const uint32_t kSampleFlagsPresentMask = 0x000400;
  static const uint32_t kSampleCompositionPresentMask = 0x00800;

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t sample_count_;
  int32_t data_offset_;
  uint32_t first_sample_flags_;
  std::vector<TrackRun> track_runs_;
};
}  //  namespace dash2hls

#endif  // _DASH2HLS_TRUN_CONTENTS_H_
