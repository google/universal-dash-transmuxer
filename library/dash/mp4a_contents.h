#ifndef _DASH2HLS_MP4A_CONTENTS_H_
#define _DASH2HLS_MP4A_CONTENTS_H_

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

// SampleEntryBox containing the audio codec information.

#include <string>

#include "library/dash/box_type.h"
#include "library/dash/audio_sample_entry_contents.h"
#include "library/dash/esds_contents.h"

namespace dash2hls {

class EsdsContents;

class Mp4aContents : public AudioSampleEntryContents {
 public:
  explicit Mp4aContents(uint64_t stream_position)
      : AudioSampleEntryContents(BoxType::kBox_mp4a, stream_position) {}
  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const {return "Audio Decode Information";}

  uint8_t get_audio_object_type() const {
    return esds_->get_audio_object_type();
  }

  uint8_t get_sampling_frequency_index() const {
    return esds_->get_sampling_frequency_index();
  }

  uint8_t get_channel_config() const {return esds_->get_channel_config();}

  const std::vector<uint8_t>& get_audio_config() const {
    return esds_->get_audio_config();
  }

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  const EsdsContents* esds_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_MP4A_CONTENTS_H_
