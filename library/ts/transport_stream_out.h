#ifndef _DASH2HLS_TRANSPORT_STREAM_OUT_H_
#define _DASH2HLS_TRANSPORT_STREAM_OUT_H_

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

// TransportStreamOut takes mp4 samples and turns them into TS samples.
//
// Expected usage is to call TransportStreamOut::ProcessSample.
// All other routines are exposed for unit testing.

#include <vector>

#include "include/DashToHlsApi.h"
#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"
#include "library/ps/nalu.h"
#include "library/ps/pes.h"

namespace dash2hls {

class TransportStreamOut {
 public:
  TransportStreamOut() : nalu_length_(0),
                         has_video_(false),
                         has_audio_(false),
                         audio_object_type_(0),
                         sampling_frequency_index_(0),
                         channel_config_(0),
                         pmt_continuity_counter(0),
                         pat_continuity_counter(0),
                         video_continuity_counter(0),
                         audio_continuity_counter(0) {
  }

  enum {
    kPidPat = 0x00,
    kPidPmt = 0x20,
    kPidVideo = 0x21,
    kPidAudio = 0x22,
  };

  // TODO(justsomeguy) this interface requires an extra copy of input because
  // it's const.  See about doing it in place.
  void ProcessSample(const uint8_t* input, size_t input_length,
                     bool is_video,
                     bool is_sync_sample,
                     uint64_t pts, uint64_t dts, uint64_t scr,
                     uint64_t duration,
                     std::vector<uint8_t>* out);

  void set_has_video(bool flag) {has_video_ = flag;}
  void set_nalu_length(size_t nalu_length) {nalu_length_ = nalu_length;}
  void set_sps_pps(const std::vector<uint8_t>& sps_pps) {sps_pps_ = sps_pps;}

  void set_has_audio(bool flag) {has_audio_ = flag;}
  void set_audio_object_type(uint8_t type) {audio_object_type_ = type;}
  void set_sampling_frequency_index(uint8_t index) {
    sampling_frequency_index_ = index;
  }
  void set_channel_config(uint8_t config) {channel_config_ = config;}
  void set_audio_config(const uint8_t config[2]);
  const std::vector<uint8_t>& get_audio_pmt() {return audio_pmt_;}

 protected:
  void PreprocessNalus(std::vector<uint8_t>* buffer, bool* has_aud,
                       nalu::PicType* pic_type);
  void AddNeededNalu(std::vector<uint8_t>* buffer, nalu::PicType pic_type,
                     bool is_sync_sample);
  void ConvertLengthToStartCode(std::vector<uint8_t>* buffer);
  void OutputRawDataOverTS(const uint8_t* data, size_t length, uint16_t pid,
                           uint16_t* continuity_counter,
                           std::vector<uint8_t>* out);
  void OutputPesOverTS(const PES& data, uint16_t pid, int64_t pcr,
                       uint16_t* continuity_counter,
                       std::vector<uint8_t>* out);
  const uint8_t* GetPmtAudio() const;
  void FrameAudio(const uint8_t* input, size_t input_length,
                  std::vector<uint8_t>* output) const;

 protected:
  static const uint8_t kID3AudioTimeTag[73];
  static const uint8_t kPat[17];
  static const uint8_t kPmtVideo[22];
  static const uint8_t kPmtAudio[59];

 private:
  size_t nalu_length_;

  bool has_video_;
  std::vector<uint8_t> sps_pps_;

  bool has_audio_;
  uint8_t audio_object_type_;
  uint8_t sampling_frequency_index_;
  uint8_t channel_config_;
  std::vector<uint8_t> audio_pmt_;

  uint16_t pmt_continuity_counter;
  uint16_t pat_continuity_counter;
  uint16_t video_continuity_counter;
  uint16_t audio_continuity_counter;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_TRANSPORT_STREAM_OUT_H_
