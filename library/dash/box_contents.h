#ifndef _DASH2HLS_BOX_CONTENTS_H_
#define _DASH2HLS_BOX_CONTENTS_H_

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

// Superclass of each Box type.

#include <stdint.h>
#include <string>

namespace dash2hls {

class DashParser;

class BoxContents {
  friend class Box;
 public:
  BoxContents(uint32_t box_type, uint64_t stream_position);
  virtual ~BoxContents();
  const DashParser* get_dash_parser() const {
    return dash_parser_;
  }
  uint64_t get_stream_position() const {return stream_position_;}

  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const;

 protected:
  // Returns the bytes parsed.  Most boxes parse length bytes always.
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 protected:
  uint64_t stream_position_;
  DashParser* dash_parser_;

 private:
  uint32_t box_type_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_BOX_CONTENTS_H_
