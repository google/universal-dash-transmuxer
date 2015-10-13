#ifndef _DASH2HLS_AUDIO_SAMPLE_ENTRY_CONTENTS_H_
#define _DASH2HLS_AUDIO_SAMPLE_ENTRY_CONTENTS_H_

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

// Sample box containing the codec information.

#include <string>

#include "library/dash/elementary_stream_descriptor.h"
#include "library/dash/sample_entry_contents.h"

namespace dash2hls {

class AudioSampleEntryContents : public SampleEntryContents {
 public:
  AudioSampleEntryContents(uint32_t type, uint64_t stream_position)
      : SampleEntryContents(type, stream_position) {}
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "AudioSampleEntry";}

  uint16_t get_channel_count() const {
    return channel_count_;
  }

  uint16_t get_sample_size() const {
    return sample_size_;
  }

  uint32_t get_sample_rate() const {
    return sample_rate_ >> 16;
  }

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  uint16_t channel_count_;
  uint16_t sample_size_;
  uint32_t sample_rate_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_AUDIO_SAMPLE_CONTENTS_H_
