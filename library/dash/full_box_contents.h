#ifndef _DASH2HLS_FULL_BOX_CONTENTS_H_
#define _DASH2HLS_FULL_BOX_CONTENTS_H_

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

// A FullBox is a box with box specific contents.  All full boxes start with
// a version and flags.  The Parse is always 4 bytes.

#include <string>

#include "library/dash/box_contents.h"

namespace dash2hls {

class FullBoxContents : public BoxContents {
 public:
  enum Version {
    kVersion0 = 0,
    kVersion1 = 1,
    kVersionLast
  };

  static const uint32_t kFullBoxHeaderSize = sizeof(uint32_t);
  FullBoxContents(uint32_t type, uint64_t stream_position) :
      BoxContents(type, stream_position) {}

  virtual size_t Parse(const uint8_t* buffer, size_t length);
  virtual std::string PrettyPrint(std::string indent) const;

 protected:
  Version version_;
  uint32_t flags_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_FULL_BOX_CONTENTS_H_
