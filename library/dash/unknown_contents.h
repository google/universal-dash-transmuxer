#ifndef _DASH2HLS_UNKNOWN_CONTENTS_H_
#define _DASH2HLS_UNKNOWN_CONTENTS_H_

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

// Not really unknown, more an irrelevant Box not needed for turning DASH
// into HLS.  See box_contents.h for comments.

#include <string>

#include "library/dash/box_contents.h"

namespace dash2hls {

class UnknownContents : public BoxContents {
 public:
  explicit UnknownContents(uint64_t position) : BoxContents(0, position),
  bytes_ignored_(0) {
  }
  virtual std::string BoxName() const {return "Unknown";}

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  size_t bytes_ignored_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_UNKNOWN_CONTENTS_H_
