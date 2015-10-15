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

#include "library/dash/sidx_contents.h"

#include <stdio.h>

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// Currently only parses version 1 of a sidx box.
//
// See ISO 14496-12 for details.
// aligned(8) class SegmentIndexBox extends FullBox(‘sidx’, version, 0) {
//   unsigned int(32) reference_ID;
//   unsigned int(32) timescale;
//   if (version==0) {
//     unsigned int(32) earliest_presentation_time;
//     unsigned int(32) first_offset;
//   } else {
//     unsigned int(64) earliest_presentation_time;
//     unsigned int(64) first_offset;
//   }
//   unsigned int(16) reserved = 0;
//   unsigned int(16) reference_count;
//   for(i=1; i <= reference_count; i++) {
//     bit (1) reference_type;
//     unsigned int(31) referenced_size;
//     unsigned int(32) subsegment_duration;
//     bit(1) starts_with_SAP;
//     unsigned int(3) SAP_type;
//     unsigned int(28) SAP_delta_time;
//   }
// }
size_t SidxContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, 8 * sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 32 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  reference_id_ = ntohlFromBuffer(ptr);
  ptr += sizeof(reference_id_);
  timescale_ = ntohlFromBuffer(ptr);
  ptr += sizeof(timescale_);
  if (version_ == 0) {
    earliest_presentation_time_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t );
    first_offset_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
  } else {
    earliest_presentation_time_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint64_t);
    first_offset_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint64_t);
    return length;
  }
  ptr += sizeof(uint16_t);
  reference_count_ = ntohsFromBuffer(ptr);
  ptr += sizeof(reference_count_);
  uint64_t location_of_moof = stream_position_ + length + first_offset_ +
      Box::kBoxHeaderSize;
  uint32_t next_start_time = earliest_presentation_time_;
  for (uint32_t count = 0; count < reference_count_; ++count) {
    Reference reference;
    reference.size_ = ntohlFromBuffer(ptr);
    ptr += sizeof(reference.size_);
    reference.segment_index_type_ = (reference.size_ >> 31);
    reference.subsegment_duration_ = ntohlFromBuffer(ptr);
    ptr += sizeof(reference.subsegment_duration_);
    reference.sap_delta_time = ntohlFromBuffer(ptr);
    ptr += sizeof(reference.sap_delta_time);
    reference.starts_with_sap_ = reference.sap_delta_time >> 31;
    reference.sap_type_ = (reference.sap_delta_time >> 28) & 0x7;
    reference.sap_delta_time &= 0x0fffffff;
    references_.push_back(reference);

    DashToHlsSegment location;
    location.start_time = next_start_time;
    location.duration = reference.subsegment_duration_;
    location.timescale = timescale_;
    next_start_time += location.duration;
    location.location = location_of_moof;
    location.length = reference.size_;
    location_of_moof += location.length;
    locations_.push_back(location);
  }
  return ptr - buffer;
}

std::string SidxContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " ReferenceID: " + PrettyPrintValue(reference_id_) +
      " Timescale: " + PrettyPrintValue(timescale_) + " Presentation Time: " +
      PrettyPrintValue(earliest_presentation_time_) + " First Offset: " +
      PrettyPrintValue(first_offset_) + " References: " +
      PrettyPrintValue(reference_count_);

  if (g_verbose_pretty_print) {
    for (std::vector<Reference>::const_iterator iter = references_.begin();
         iter != references_.end(); ++iter) {
      result += "\n" + indent + "  ";
      result += (iter->segment_index_type_ ? "Segment":"Media");
      result += " Size: " + PrettyPrintValue(iter->size_) + " Sub duration: ";
      result += PrettyPrintValue(iter->subsegment_duration_);
      result += (iter->starts_with_sap_ ? " SAP ":" ");
      result += PrettyPrintValue(iter->sap_type_) + " " +
          PrettyPrintValue(iter->sap_delta_time);
    }
    for (std::vector<DashToHlsSegment>::const_iterator
             iter = locations_.begin(); iter != locations_.end(); ++iter) {
      result += "\n" + indent + " time:" + PrettyPrintValue(iter->start_time)
          + "-" + PrettyPrintValue(iter->duration) + " position:" +
          PrettyPrintValue(iter->location) + "-"
          + PrettyPrintValue(iter->length);
    }
  }
  return result;
}
}  // namespace dash2hls
