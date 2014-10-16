#ifndef _DASH2HLS_PSSH_CONTENTS_H_
#define _DASH2HLS_PSSH_CONTENTS_H_

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

// pssh is the Protection System Specification Header.  While the pssh has a
// structure the CDM code takes the entire contents (without the mp4 header)
// and parses it.  Parsing in this box is just to provide a human readable
// output for diagnostics.

#include <string>
#include <vector>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class PsshContents : public FullBoxContents {
 public:
  explicit PsshContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_pssh, stream_position) {}

  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {
    return "ProtectionSystemSpecificHeader";
  }

  const std::vector<uint8_t> get_full_box() const {return full_box_;}
  const std::vector<uint8_t> get_contents() const {return contents_;}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint8_t system_id_[16];
  std::vector<uint8_t> full_box_;
  std::vector<uint8_t> data_;
  std::vector<uint8_t> contents_;
};
}  //  namespace dash2hls

#endif  // _DASH2HLS_PSSH_CONTENTS_H_
