#ifndef _DASH2HLS_EMSG_CONTENTS_H_
#define _DASH2HLS_EMSG_CONTENTS_H_

/*
 Copyright 2015 Google Inc. All rights reserved.

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

#include <string>
#include <vector>

#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"

namespace dash2hls {

class EmsgContents : public FullBoxContents {
 public:
  explicit EmsgContents(uint64_t stream_position)
      : FullBoxContents(BoxType::kBox_emsg, stream_position) {}

  const std::string& get_scheme_id_uri() const { return scheme_id_uri_; }
  const std::string get_value() const { return value_; }
  const uint32_t& get_timescale() const { return timescale_; }
  const uint32_t& get_presentation_time_delta() const {
    return presentation_time_delta_;
  }
  const uint32_t& get_event_duration() const { return event_duration_; }
  const uint32_t& get_id() const { return id_; }
  const std::vector<uint8_t>& get_message_data() const { return message_data_; }

  virtual std::string PrettyPrint(std::string indent) const;
  virtual std::string BoxName() const { return "EventMessage"; }

 protected:
  virtual size_t Parse(const uint8_t* buffer, size_t length);

 private:
  std::string scheme_id_uri_;
  std::string value_;
  uint32_t timescale_;
  uint32_t presentation_time_delta_;
  uint32_t event_duration_;
  uint32_t id_;
  std::vector<uint8_t> message_data_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_EMSG_CONTENTS_H_
