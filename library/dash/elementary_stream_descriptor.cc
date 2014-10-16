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

#include "library/dash/elementary_stream_descriptor.h"

#include <string>

#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

std::string ElementaryStreamDescriptor::PrettyPrint(std::string indent) const {
  std::string result = BaseDescriptor::PrettyPrint(indent);
  result += std::string(" ID:") + PrettyPrintValue(id_);
  result += std::string(" Flags:") + PrettyPrintValue(id_);
  if (HasStreamDependsFlag()) {
    result += std::string(" DependsOnId:") + PrettyPrintValue(id_);
  }
  if (HasUrl()) {
    result += std::string(" Url:") +
        std::string(reinterpret_cast<const char *>(url_.data()));
  }
  if (HasOcrStream()) {
    result += std::string(" OcrStreamFlag:") + PrettyPrintValue(ocr_id_);
  }
  result += std::string(" Priority:") +
      PrettyPrintValue(StreamPriority()) + "\n";
  std::string subindent = indent + "  ";
  result += decoder_descriptor_.PrettyPrint(subindent) + "\n";
  result += sl_descriptor_.PrettyPrint(subindent) + "\n";
  result += ipi_ptr_.PrettyPrint(subindent) + "\n";
  result += language_.PrettyPrint(subindent) + "\n";
  result += qos_.PrettyPrint(subindent) + "\n";
  result += registration_.PrettyPrint(subindent) + "\n";
  result += extension_.PrettyPrint(subindent);
  return result;
}

// See ISO 14496-15 and 14495-12 for details.
//
// class ES_Descriptor extends BaseDescriptor : bit(8) tag=ES_DescrTag {
//   bit(16) ES_ID;
//   bit(1) streamDependenceFlag;
//   bit(1) URL_Flag;
//   bit(1) OCRstreamFlag;
//   bit(5) streamPriority;
//   if (streamDependenceFlag)
//     bit(16) dependsOn_ES_ID;
//   if (URL_Flag) {
//     bit(8) URLlength;
//     bit(8) URLstring[URLlength];
//   }
//   if (OCRstreamFlag)
//     bit(16) OCR_ES_Id;
//   DecoderConfigDescriptor decConfigDescr;
//   if (ODProfileLevelIndication==0x01) //no SL extension.
//   {
//     SLConfigDescriptor slConfigDescr;
//   }
//   else // SL extension is possible.
//   {
//     SLConfigDescriptor slConfigDescr;
//   }
//   IPI_DescrPointer ipiPtr[0 .. 1];
//   IP_IdentificationDataSet ipIDS[0 .. 255];
//   IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];
//   LanguageDescriptor langDescr[0 .. 255];
//   QoS_Descriptor qosDescr[0 .. 1];
//   RegistrationDescriptor regDescr[0 .. 1];
//   ExtensionDescriptor extDescr[0 .. 255];
// }
size_t ElementaryStreamDescriptor::Parse(const uint8_t* buffer,
                                         size_t length) {
  const uint8_t* bptr = buffer;
  bptr += ParseHeader(buffer, length);
  if (bptr == buffer) {
    return DashParser::kParseFailure;
  }
  if (get_tag() != kES_DescrTag) {
    DASH_LOG("Expected Elementary Stream Descriptor",
             "Wrong BaseDescriptor",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (!EnoughBytesToParse(bptr - buffer, 2 * sizeof(uint8_t), length)) {
    DASH_LOG("Elementary Stream Descriptor too short",
             "Need an id and flags",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  id_ = ntohsFromBuffer(bptr);
  bptr += sizeof(id_);
  flags_ = *bptr;
  ++bptr;
  if (HasStreamDependsFlag()) {
    if (!EnoughBytesToParse(bptr - buffer, sizeof(uint8_t), length)) {
      DASH_LOG("Elementary Stream Descriptor too short",
               "Needs a StreamDepends_On_ES_ID",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    depends_on_id_ = *bptr;
    ++bptr;
  }
  if (HasUrl()) {
    if (!EnoughBytesToParse(bptr - buffer, sizeof(uint8_t), length) ||
        !EnoughBytesToParse(bptr - buffer, *bptr + 1, length)) {
      DASH_LOG("Elementary Stream Descriptor too short",
               "URL is short.",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    url_.resize(*bptr);
    ++bptr;
    memcpy(&url_, bptr, url_.size());
    bptr += url_.size();
  }
  if (HasOcrStream()) {
    if (!EnoughBytesToParse(bptr - buffer, sizeof(uint16_t), length)) {
      DASH_LOG("Elementary Stream Descriptor too short",
               "Needs OCR_ES_Id",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    ocr_id_ = ntohsFromBuffer(bptr);
    bptr += sizeof(ocr_id_);
  }
  if (length < static_cast<size_t>(bptr - buffer)) {
    DASH_LOG("Elementary Stream Descriptor too short",
             "Buffer overrun",
             DumpMemory(buffer, length).c_str());
  }
  size_t bytes_left = length - (bptr - buffer);
  size_t bytes_parsed = 0;
  if (bytes_left > 0) {
    bytes_parsed = decoder_descriptor_.Parse(bptr, bytes_left);
    if (bytes_parsed == 0) {
      return DashParser::kParseFailure;
    }
    bptr += bytes_parsed;
    bytes_left -= bytes_parsed;
  }
  if (bytes_left > 0) {
    bytes_parsed = sl_descriptor_.Parse(bptr, length);
    if (bytes_parsed == 0) {
      return DashParser::kParseFailure;
    }
    bptr += bytes_parsed;
    bytes_left -= bytes_parsed;
  }
  if (bytes_left > 0) {
    bytes_parsed = ipi_ptr_.Parse(bptr, length);
    if (bytes_parsed == 0) {
      return DashParser::kParseFailure;
    }
    bptr += bytes_parsed;
    bytes_left -= bytes_parsed;
  }
  if (bytes_left > 0) {
    bytes_parsed = language_.Parse(bptr, length);
    if (bytes_parsed == 0) {
      return DashParser::kParseFailure;
    }
    bptr += bytes_parsed;
    bytes_left -= bytes_parsed;
  }
  if (bytes_left > 0) {
    bytes_parsed = qos_.Parse(bptr, length);
    if (bytes_parsed == 0) {
      return DashParser::kParseFailure;
    }
    bptr += bytes_parsed;
    bytes_left -= bytes_parsed;
  }
  if (bytes_left > 0) {
    bytes_parsed = registration_.Parse(bptr, length);
    if (bytes_parsed == 0) {
      return DashParser::kParseFailure;
    }
    bptr += bytes_parsed;
    bytes_left -= bytes_parsed;
  }
  if (bytes_left > 0) {
    bytes_parsed = extension_.Parse(bptr, length);
    if (bytes_parsed == 0) {
      return DashParser::kParseFailure;
    }
    bptr += bytes_parsed;
    bytes_left -= bytes_parsed;
  }
  if (bytes_left) {
    DASH_LOG("ElementaryStream too long",
             "ElementaryStream did not parse all bytes",
             DumpMemory(bptr, bytes_left).c_str());
  }

  return bptr - buffer;
}
}  // namespace dash2hls
