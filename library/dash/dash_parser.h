#ifndef _DASH2HLS_DASHPARSER_H_
#define _DASH2HLS_DASHPARSER_H_

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

// Main interface for a DASH parser.  Can parse any DASH box containing
// other boxes and will recurse to create additional DashParser's as
// needed.
//
// You can load as much DASH content as you want.  The more you load the
// more RAM you will need.  Future versions of the DashParser will allow
// processing on the DOM in a way that allows reloading of data as needed.
// In practical use it is irrelevant because only the first 1-2K needs to be
// parsed to get the sidx.  From the sidx only individually segments would ever
// be loaded.  These segments tend to be 3-10 seconds of video.
//
// DashParser includes routines to diagnose DASH content.
//
// Example:
//   DashParser parser;
//   parser.Parse(GetDashContents());
//   SidxContents* parser.GetBoxContents(kType_sidx);
//
// Defensive coding:
// The DashParser does its best to make sure entire boxes are parsed at once.
// This removes the vast majority of buffer overruns.  The DashParser does
// not have knowledge of the internal structure of boxes, however, so each
// box should use EnoughBytesToParse before reading the internal box.
//
// Any Parse that returns 0 bytes is by definition a failure and should be
// treated as aborting parsing.
//
// Diagnostic tools can ignore the 0 returned by a BoxContent as the length
// of the box should let the parser continue on to the next box.  Regular media
// should just abort.

#include <string>
#include <vector>

#include "library/dash/box.h"

namespace dash2hls {

class BoxType;

class DashParser {
 public:
  DashParser();

  enum {
    // Any parse returning kParseFailure instead of a size is considered a
    // failure.  Parsing should abort.
    kParseFailure = 0,
  };

  // Syncs the DashParser to the stream_position.  This is for starting parsing
  // in the middle of a stream.
  void set_current_position(uint64_t stream_position) {
    current_stream_position_ = stream_position;
  }

  void set_default_iv_size(size_t size) {default_iv_size_ = size;}
  size_t get_default_iv_size() const {return default_iv_size_;}

  // Parses the next |length| bytes of |buffer|.  Parse expects repeated calls
  // to not skip any data.  Memory pointed to by |buffer| is never needed
  // after Parse returns and may be freed.
  //
  // Returns bytes parsed, which will always be length as anything not parsed
  // by this call is placed in the spillover for the next pass.
  size_t Parse(const uint8_t* buffer, size_t length);

  // Finds at the top level the first box of box_type.  Returns nullptr if
  // no boxes are found.  The parser owns the memory of the Box.
  const Box* Find(const BoxType::Type& box_type) const;
  // Recursively finds the first box of box_type.  Returns nullptr if no boxes
  // are found.  The parser owns the memory of the Box.
  const Box* FindDeep(const BoxType::Type& box_type) const;
  // Finds all the boxes of box_type at the top level. The parser owns the
  // memory of any box in the vector.
  const std::vector<const Box*> FindAll(const BoxType::Type& box_type) const;
  // Finds all the boxes of box_type recursively. The parser owns the
  // memory of any box in the vector.
  const std::vector<const Box*> FindDeepAll(
      const BoxType::Type& box_type) const;

  // Debugging routine for diagnostics.
  std::string PrettyPrint(std::string indent) const;

 protected:
  // Spillover is used to handle extra bytes from a call to Parse before an
  // entire box is seen.  Spillover has been optimized to be only used when
  // needed.  This avoids copying extra bytes.
  //
  // The spillover is meant to concatenate an entire Box to simplify Box
  // parsing.  It should be used only when a box is split across multiple
  // calls to parse.
  void AddSpillover(const uint8_t* buffer, size_t length);

  // AddToSpilloverIfNeeded only adds bytes to the spillover if it's in use.
  // If there is no existing spillover then no bytes are added and the caller
  // should process the original buffer.
  size_t AddToSpilloverIfNeeded(const uint8_t* buffer, size_t length);

 private:
  std::vector<uint8_t> spillover_;
  std::vector<Box> boxes_;
  Box* current_box_;
  uint64_t current_stream_position_;
  size_t default_iv_size_;
};

}  // namespace dash2hls

#endif  // _DASH2HLS_DASHPARSER_H_
