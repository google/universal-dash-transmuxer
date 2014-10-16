#ifndef _DASH2HLS_DASH_AVCC_CONTENTS_H_
#define _DASH2HLS_DASH_AVCC_CONTENTS_H_

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

// Sample box containing the AVC decoder information.

#include <string>
#include <vector>

#include "library/dash/avc_decoder_configuration_record.h"
#include "library/dash/box_contents.h"
#include "library/dash/box_type.h"

namespace dash2hls {

class AvcCContents : public BoxContents {
 public:
  explicit  AvcCContents(uint64_t stream_position)
      : BoxContents(BoxType::kBox_avcC, stream_position) {}
  const std::vector<std::vector<uint8_t> >&
      get_sequence_parameter_sets() const {
    return avc_record_.get_sequence_parameter_sets();
  }
  const std::vector<std::vector<uint8_t> >&
      get_picture_parameter_sets() const {
    return avc_record_.get_picture_parameter_sets();
  }
  size_t GetNaluLength() const {
    return avc_record_.GetNaluLength();
  }
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "AVC1 Decoder Configuration";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  AvcDecoderConfigurationRecord avc_record_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_DASH_AVCC_CONTENTS_H_
