#ifndef _DASH2HLS_SAIO_CONTENTS_H_
#define _DASH2HLS_SAIO_CONTENTS_H_

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

// Samples can have auxiliary information stored in the content.  The
// information per sample is stored in either a continuous block or spread
// throughout the data.  The entry_count of 1 means continous, the only other
// legal value is the entry_count in the trun.

#include <string>
#include <vector>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class SaioContents : public FullBoxContents {
 public:
  explicit SaioContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_saio, stream_position) {}

  // Fields are optional.
  bool IsAuxInfoPresent() const;

  uint32_t get_aux_info_type() {return aux_info_type_;}
  uint32_t get_aux_info_type_parameter() {return aux_info_type_parameter_;}

  const std::vector<uint64_t>& get_offsets() const {return offsets_;}

  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {
    return "SampleAuxiliaryInformationOffsetsBox";
  }

 protected:
  static const uint32_t kAuxInfoPresentMask = 0x000001;

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t aux_info_type_;
  uint32_t aux_info_type_parameter_;
  std::vector<uint64_t> offsets_;
};
}  //  namespace dash2hls

#endif  // _DASH2HLS_SAIO_CONTENTS_H_
