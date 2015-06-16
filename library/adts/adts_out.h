#ifndef _DASH2HLS_ADTS_OUT_H_
#define _DASH2HLS_ADTS_OUT_H_

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

// AdtsOut takes mp4 samples and turns them into ADTS samples.
//
// Expected usage is to call AdtsOut::ProcessSample.
// All other routines are exposed for unit testing.

#include <vector>

#include "include/DashToHlsApi.h"
#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"
#include "library/ps/nalu.h"
#include "library/ps/pes.h"

namespace dash2hls {

class AdtsOut {
public:
  void AddTimestamp(uint64_t pts, std::vector<uint8_t>* out);
  // TODO(justsomeguy) this interface requires an extra copy of input because
  // it's const.  See about doing it in place.
  void ProcessSample(const uint8_t* input, size_t input_length,
                     std::vector<uint8_t>* out);

  void set_audio_object_type(uint8_t type) {audio_object_type_ = type;}
  void set_channel_config(uint8_t config) {channel_config_ = config;}
  void set_sampling_frequency_index(uint8_t index) {
    sampling_frequency_index_ = index;
  }

protected:
  static const uint8_t kID3AudioTimeTag[73];

private:
  uint8_t audio_object_type_;
  uint8_t sampling_frequency_index_;
  uint8_t channel_config_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_ADTS_OUT_H_
