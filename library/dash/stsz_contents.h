#ifndef _DASH2HLS_STSZ_CONTENTS_H_
#define _DASH2HLS_STSZ_CONTENTS_H_

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

// Sample Table Box contains a list of all samples.

#include <string>
#include <vector>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class StszContents : public FullBoxContents {
 public:
  explicit StszContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_stsz, stream_position) {}
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "SampleTable";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t sample_size_;  // Default sample size
  uint32_t sample_count_;
  std::vector<uint32_t> samples_sizes_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_STSZ_CONTENTS_H_
