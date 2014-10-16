#ifndef _DASH2HLS_PSM_H_
#define _DASH2HLS_PSM_H_

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

// Every segment has a PSM following the system header.
// See ISO-14966-10 for details on its internals.

#include <stdint.h>
#include <string>
#include <vector>

namespace dash2hls {

class PSM {
 public:
  static const uint8_t kVideoAlignmentDescriptor[3];
  static const uint8_t kAudioAlignmentDescriptor[3];

 public:
  PSM();

  size_t GetSize() const;

  size_t Write(uint8_t* buffer, uint32_t max_length) const;
  void set_current_next_indicator(bool current_next) {
    current_next_indicator_ = current_next;
  }
  void set_psm_version(uint8_t psm_version) {psm_version_ = psm_version;}
  void AddDescriptor(const uint8_t* descriptor, uint16_t size);
  void AddElementaryStream(uint8_t stream_id, const uint8_t stream_type);
  void AddElementaryStreamDescriptor(uint8_t stream_id,
                                     const uint8_t* descriptor, uint16_t size);

 private:
  struct ElementaryStreamInfo {
    uint8_t stream_id;
    uint8_t stream_type;
    std::vector<uint8_t> descriptors;
  };
  bool current_next_indicator_;
  uint8_t psm_version_;
  std::vector<uint8_t> descriptors_;
  std::vector<ElementaryStreamInfo> elementary_streams_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_PSM_H_
