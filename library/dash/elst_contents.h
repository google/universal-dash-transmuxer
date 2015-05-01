#ifndef _DASH2HLS_ELST_CONTENTS_H_
#define _DASH2HLS_ELST_CONTENTS_H_

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

class ElstContents : public FullBoxContents {
 public:
  class Entry {
   public:
    uint64_t segment_duration_;
    int64_t media_time_;
    int16_t media_rate_integer_;
    int16_t media_rate_fraction_;
  };

  explicit ElstContents(uint64_t stream_position)
    : FullBoxContents(BoxType::kBox_elst, stream_position) {}

  const std::vector<Entry>& get_entries() const {return entries_;}

  virtual std::string PrettyPrintEntry(const Entry& entry) const;
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "EditList";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t entry_count_;
  std::vector<Entry> entries_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_ELST_CONTENTS_H_
