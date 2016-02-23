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

#ifndef DASHTOHLS_DASHTOHLS_SESSION_H_
#define DASHTOHLS_DASHTOHLS_SESSION_H_

#include <map>

#include "include/DashToHlsApi.h"
#include "library/dash/dash_parser.h"
#include "library/dash/tenc_contents.h"

namespace dash2hls {
// Internal Session object.  Tracks all information used by the calls.
class Session {
 public:
  Session() :
      is_video_(false), is_encrypted_(false), encrypt_output_(false),
      pssh_handler_(nullptr), decryption_handler_(nullptr),
      default_iv_size_(0), nalu_length_(0), audio_object_type_(0),
      sampling_frequency_index_(0), channel_config_(0),
      pssh_context_(nullptr), decryption_context_(nullptr),
      timescale_(0), trex_default_sample_duration_(0) {
  }
  bool encrypt_output_;
  bool is_encrypted_;
  bool is_video_;
  CENC_PsshHandler pssh_handler_;
  CENC_DecryptionHandler decryption_handler_;
  DashToHlsIndex index_;
  DashParser parser_;
  size_t default_iv_size_;
  std::map<uint32_t, std::vector<uint8_t> > output_;
  std::vector<uint8_t> reencryption_key;

  // Video specific settings.
  std::vector<uint8_t> sps_pps_;
  size_t nalu_length_;

  // Audio specific settings.
  uint8_t audio_object_type_;
  uint8_t sampling_frequency_index_;
  uint8_t channel_config_;
  uint8_t audio_config_[2];
  DashToHlsContext pssh_context_;
  DashToHlsContext decryption_context_;
  uint64_t timescale_;
  uint8_t key_id_[TencContents::kKidSize];
  uint64_t trex_default_sample_duration_;
};
}  // namespace dash2hls

#endif  // DASHTOHLS_DASHTOHLS_SESSION_H_
