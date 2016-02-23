#ifndef _DASH2HLS_BOX_H_
#define _DASH2HLS_BOX_H_

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

// An mp4 box (atom) containing box specific data or more boxes.  Box specific
// data is contained in a subclass of BoxContents.
//
// To optimize parsing the Box is parsed in steps.  The first step reads the
// type and size of the box.  After that it Parses the entire BoxContents at
// once.  BytesNeededToContinue returns how many bytes the Box Parse requires.
//
// Example:
// uint8_t* dash_content = GetDashContent();
// size_t dash_size = GetDashSize();
// size_t bytes_parsed = 0;
// Box box;
// while (!box.DoneParsing) {
//   if (box.BytesNeededToContinue() > dash_size - bytes_parsed) {
//     return ERROR;
//   }
//   bytes_parsed += box.Parse(dash_content + bytes_parsed,
//                             dash_size - bytes_parsed);
// }

#include <string>

#include "library/compatibility.h"
#include "library/dash/box_contents.h"
#include "library/dash/box_type.h"

namespace dash2hls {

class Box {
 public:
  enum {
    kBoxHeaderSize = sizeof(uint32_t) * 2
  };
  // The |start_position| is where in the stream this Box starts.
  explicit Box(uint64_t start_position);

  bool DoneParsing() const;

  // Minimum number of bytes needed to call Parse.
  size_t BytesNeededToContinue() const;
  // Handle the next phase of parsing returning the number of bytes parsed.
  // length must be at least as long as BytesNeededToContinue.
  size_t Parse(const uint8_t* buffer, size_t length);

  const BoxContents* get_contents() const {return contents_.get();}
  const BoxType& get_type() const {return type_;}
  // Debugging routine for diagnostics.
  std::string PrettyPrint(std::string indent) const;

 protected:
  enum State {
    kInitialState = 0,
    kReadingType,
    kReadingBytes,
    kParsed,
    kLast
  };
  void CreateContentsObject();

 private:
  State state_;
  size_t size_;
  BoxType type_;
  size_t bytes_read_;
  shared_ptr<BoxContents> contents_;
  uint64_t stream_position_;
};
}  // namespace dash2hls

#endif  // DASH2HLS_BOX_H_
