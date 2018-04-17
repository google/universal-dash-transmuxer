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

#include "library/dash/prft_contents.h"

#include <sstream>

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

/**
 * See ISO/IEC 14496-12:2015(E): 8.16.5 Producer Reference Time Box for details
 * on the format of this box. This also calls FullBoxContents::Parse for setup.
 *
 * aligned(8) class ProducerReferenceTimeBox extends FullBox(‘prft’, version, 0)
 * {
 *   unsigned int(32) reference_track_ID;
 *   unsigned int(64) ntp_timestamp;
 *   if (version==0) {
 *     unsigned int(32) media_time;
 *   } else {
 *     unsigned int(64) media_time;
 *   }
 * }
 */
size_t PrftContents::Parse(const uint8_t* buffer, size_t length) {
  size_t parsed = 0;
  size_t result = FullBoxContents::Parse(buffer, length);
  if (result == DashParser::kParseFailure) {
    return result;
  }

  if (version_ != 0 && version_ != 1) {
    char message[128];
    char reason[512];
    snprintf(message, sizeof(message), "%s unknown version", BoxName().c_str());
    snprintf(reason, sizeof(reason), "Unrecognised version: %d", version_);
    DASH_LOG(message, reason, DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }

  parsed += result;

  size_t needed = 4 /* track_ID */ + 8 /* ntp_time */ + 4 /* media_time_v0 */;
  if (version_ == 1) {
    needed += 4 /* media_time_v1 */;
  }

  if (!EnoughBytesToParse(parsed, needed, length)) {
    char message[128];
    char reason[512];
    snprintf(message, sizeof(message), "%s too short", BoxName().c_str());
    snprintf(reason, sizeof(reason),
             "At least %zu bytes are required, only %zu available", needed,
             length - parsed);
    DASH_LOG(message, reason, DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }

  reference_track_id_ = ntohlFromBuffer(buffer + parsed);
  parsed += sizeof(reference_track_id_);  // 4

  // NTP timestamp format specifically requires it to be big endian. Don't
  // convert to host endian.
  memcpy(&ntp_timestamp_, buffer + parsed, sizeof(ntp_timestamp_));
  parsed += sizeof(ntp_timestamp_);  // 8

  if (version_ == 0) {
    media_time_ = ntohlFromBuffer(buffer + parsed);
    parsed += 4;
  } else {
    media_time_ = ntohllFromBuffer(buffer + parsed);
    parsed += 8;
  }

  return parsed;
}

std::string PrftContents::PrettyPrint(std::string indent) const {
  std::stringstream s;
  s << FullBoxContents::PrettyPrint(indent) << "\n"
    << indent << "reference_track_id: " << PrettyPrintValue(reference_track_id_)
    << "\n"
    << indent << "ntp_timestamp: " << PrettyPrintValue(ntp_timestamp_) << "\n"
    << indent << "media_time: " << PrettyPrintValue(media_time_) << "\n";
  return s.str();
}

}  // namespace dash2hls
