/*
 Copyright 2016 Google Inc. All rights reserved.

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

#include "library/dash/emsg_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

/**
 * ISO/IEC 23009-1 (2014): 5.10.3.3 Event message box
 * aligned(8) class DASHEventMessageBox
 *     extends FullBox(‘emsg’, version = 0, flags = 0) {
 *   string scheme_id_uri;
 *   string value;
 *   unsigned int(32) timescale;
 *   unsigned int(32) presentation_time_delta;
 *   unsigned int(32) event_duration;
 *   unsigned int(32) id;
 *   unsigned int(8) message_data[];
 * }
 */
size_t EmsgContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  size_t parsed = ptr - buffer;
  if (!EnoughBytesToParse(parsed, 2 * sizeof(uint8_t) + 4 * sizeof(uint32_t),
                          length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 18 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }

  size_t pos = PositionOfZeroInBuffer(buffer + parsed, length - parsed);
  if (pos == -1) {
    DASH_LOG((BoxName() + ".scheme_id_uri could not be parsed").c_str(),
             "Null terminator not found", DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  scheme_id_uri_ = std::string((const char*)buffer + parsed, pos);
  parsed += pos + 1;

  pos = PositionOfZeroInBuffer(buffer + parsed, length - parsed);
  if (pos == -1) {
    DASH_LOG((BoxName() + ".value could not be parsed").c_str(),
             "Null terminator not found", DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  value_ = std::string((const char*)buffer + parsed, pos);
  parsed += pos + 1;

  if (!EnoughBytesToParse(parsed, 4 * sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(), "Less than 16 bytes remaining",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }

  timescale_ = ntohlFromBuffer(buffer + parsed);
  parsed += sizeof(timescale_);

  presentation_time_delta_ = ntohlFromBuffer(buffer + parsed);
  parsed += sizeof(presentation_time_delta_);

  event_duration_ = ntohlFromBuffer(buffer + parsed);
  parsed += sizeof(event_duration_);

  id_ = ntohlFromBuffer(buffer + parsed);
  parsed += sizeof(id_);

  message_data_.clear();
  message_data_.insert(message_data_.end(), buffer + parsed, buffer + length);

  return length;
}

std::string EmsgContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent) + "\n";
  result += indent + "scheme_id_uri: " + scheme_id_uri_ + "\n";
  result += indent + "value: " + value_ + "\n";
  result += indent + "timescale: " + PrettyPrintValue(timescale_) + "\n";
  result += indent + "presentation_time_delta: " +
            PrettyPrintValue(presentation_time_delta_) + "\n";
  result +=
      indent + "event_duration: " + PrettyPrintValue(event_duration_) + "\n";
  result += indent + "id: " + PrettyPrintValue(id_) + "\n";
  result += indent + "message_data: " +
            PrettyPrintBuffer(&message_data_.front(), message_data_.size());
  return result;
}

}  // namespace dash2hls
