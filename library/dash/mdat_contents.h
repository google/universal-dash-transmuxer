#ifndef _DASH2HLS_MDAT_CONTENTS_H_
#define _DASH2HLS_MDAT_CONTENTS_H_

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

// Raw data samples.  The data are packed based on the contents of the other
// boxes in the traf.

#include <string>

#include "library/compatibility.h"
#include "library/dash/box_contents.h"
#include "library/dash/box_type.h"

namespace dash2hls {

class MdatContents : public BoxContents {
 public:
  explicit MdatContents(uint64_t position)
      : BoxContents(BoxType::kBox_mdat, position),
        raw_data_(nullptr),
        raw_data_length_(0) {
  }
  const uint8_t* get_raw_data() const {return raw_data_;}
  size_t get_raw_data_length() const {return raw_data_length_;}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "Media Data";}

 private:
  const uint8_t* raw_data_;
  size_t raw_data_length_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_MDAT_CONTENTS_H_
