#ifndef THIRD_PARTY_VIDEO_UDT_DASH_TRANSMUXER_LIBRARY_DASH_MSHP_CONTENTS_H_
#define THIRD_PARTY_VIDEO_UDT_DASH_TRANSMUXER_LIBRARY_DASH_MSHP_CONTENTS_H_

/*
Copyright 2016 Google Inc. All rights reserved.

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

#include "include/DashToHlsApi.h"
#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

// TODO(b/30914726): Keep this up to date with the latest spec drafts.
class MshpContents : public FullBoxContents {
 public:
  explicit MshpContents(uint64_t stream_position) :
      FullBoxContents(BoxType::kBox_st3d, stream_position) {}
  uint32_t get_crc32() const {return crc32_;}
  uint32_t get_encoding_four_cc() const {return encoding_four_cc_;}
  const uint8_t* get_raw_data() const {return raw_data_;}
  size_t get_raw_data_length() const {return raw_data_length_;}
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "MeshProjection";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t crc32_;
  uint32_t encoding_four_cc_;
  const uint8_t* raw_data_;
  size_t raw_data_length_;
};
}  // namespace dash2hls

#endif  // THIRD_PARTY_VIDEO_UDT_DASH_TRANSMUXER_LIBRARY_DASH_MSHP_CONTENTS_H_

