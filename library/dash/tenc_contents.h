#ifndef _DASH2HLS_TENC_CONTENTS_H_
#define _DASH2HLS_TENC_CONTENTS_H_

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

class TencContents : public FullBoxContents {
 public:
  enum {
    kKidSize = 16,
    kDefaultShift = 8,
    kDefaultIsEncryptedSize = 3,
  };
  explicit TencContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_stsz, stream_position) {}
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "Track Encryption Box";}
  size_t get_default_iv_size() const {return default_iv_size_;}
  const uint8_t* get_default_kid() const {return default_kid_;}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint32_t default_is_encrypted_;
  size_t default_iv_size_;
  uint8_t default_kid_[kKidSize];
};
}  // namespace dash2hls

#endif  // _DASH2HLS_TENC_CONTENTS_H_
