#ifndef _DASH2HLS_SAIZ_CONTENTS_H_
#define _DASH2HLS_SAIZ_CONTENTS_H_

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

class SaizContents : public FullBoxContents {
 public:
  enum {
    SaizRecordSize = sizeof(uint16_t) + sizeof(uint32_t),
  };
  class SaizRecord {
   public:
    size_t clear_bytes() const {
      return ntohsFromBuffer(reinterpret_cast<const uint8_t*>(storage));
    }
    size_t encrypted_bytes() const {
      return ntohlFromBuffer(reinterpret_cast<const uint8_t*>
                             (&storage[sizeof(uint16_t)]));
    }
    uint8_t storage[SaizRecordSize];
  };
  explicit SaizContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_saiz, stream_position) {}

  // Fields are optional.
  bool IsAuxInfoPresent() const;

  uint32_t get_aux_info_type() {return aux_info_type_;}
  uint32_t get_aux_info_type_parameter() {return aux_info_type_parameter_;}
  uint32_t get_default_sample_info_size() {return default_sample_info_size_;}

  const std::vector<uint8_t>& get_sizes() const {return sizes_;}

  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {
    return "SampleAuxiliaryInformationSizeBox";
  }

 protected:
  static const uint32_t kAuxInfoPresentMask = 0x000001;

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t aux_info_type_;
  uint32_t aux_info_type_parameter_;
  uint8_t default_sample_info_size_;
  std::vector<uint8_t> sizes_;
};
}  //  namespace dash2hls

#endif  // _DASH2HLS_SAIZ_CONTENTS_H_
