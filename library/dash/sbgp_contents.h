#ifndef _DASH2HLS_SBGP_CONTENTS_H_
#define _DASH2HLS_SBGP_CONTENTS_H_

/*
Copyright 2018 Google Inc. All rights reserved.

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

// Samples can have auxiliary information stored in the content.  The
// information per sample is stored at the offset described by the saio with
// sample sizes as determined by this box.
//
// Not all samples need auxiliary information but there is no skipping.  If
// there are 100 samples and only 10 sizes then the first 10 samples have info.

#include <string>
#include <vector>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"
#include "library/utilities.h"

namespace dash2hls {

class SbgpContents : public FullBoxContents {
 public:
  struct GroupEntry {
    uint32_t sample_count_;
    uint32_t group_description_index_;
  };
  explicit SbgpContents(uint8_t stream_position)
      : FullBoxContents(BoxType::kBox_sbgp, stream_position) {}

  // Fields are optional.
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {
    return "SampleAuxiliaryInformationSizeBox";
  }
  const uint32_t get_grouping_type() const { return grouping_type_; }
  const uint32_t get_sample_count() const { return entries_[0].sample_count_; }
  const uint32_t get_description_index() const {
    return entries_[0].group_description_index_;
  }

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t grouping_type_;
  uint32_t grouping_type_parameter_;
  uint32_t entry_count_;
  std::vector<GroupEntry> entries_;
};
}  //  namespace dash2hls

#endif  // _DASH2HLS_SBGP_CONTENTS_H_
