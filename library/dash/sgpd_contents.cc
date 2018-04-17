/*
Copyright 2018 Google Inc. All rights reserved.

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

#include "library/dash/sgpd_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 14496-12 for details.
// aligned(8) class SampleGroupDescriptionBox (unsigned int(32) handler_type)
// extends FullBox('sgpd', version, 0) {
//   unsigned int(32) grouping_type;
//   if (version==1) {
//     unsigned int(32) default_length;
//   }
//   if (version>=2) {
//     unsigned int(32) default_sample_description_index;
//   }
//   unsigned int(32) entry_count;
//   int i;
//   for (i = 1 ; i <= entry_count ; i++){
//     if (version==1) {
//       if (default_length==0) {
//         unsigned int(32) description_length;
//       }
//     }
//     SampleGroupEntry (grouping_type);
//     an instance of a class derived from SampleGroupEntry
//     that is appropriate and permitted for the media type
//     aligned(8) class CencSampleEncryptionInformationGroupEntry
//     extends SampleGroupEntry( ‘seig’ ) {
//       unsigned int(8) reserved = 0;
//       unsigned int(4) crypt_byte_block = 0;
//       unsigned int(4) skip_byte_block = 0;
//       unsigned int(8) isProtected;
//       unsigned int(8) Per_Sample_IV_Size;
//       unsigned int(8)[16] KID;
//       if (isProtected ==1 && Per_Sample_IV_Size == 0) {
//         unsigned int(8) constant_IV_size;
//         unsigned int(8)[constant_IV_size] constant_IV;
//       }
//     }
//   }
// }

// 'seig' numerical value for compare.
const uint32_t kSeigGroupType = 0x73656967;

size_t SgpdContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 4 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  grouping_type_ = ntohlFromBuffer(ptr);
  ptr += sizeof(uint32_t);

  if (version_ == 1) {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(), "Can not get default_length",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    default_length_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
  } else {
    default_length_ = 0;
  }

  if (version_ > 1) {
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(),
               "Can not get default_sample_description_index",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    default_sample_description_index_ = ntohlFromBuffer(ptr);
    ptr += sizeof(uint32_t);
  } else {
    default_sample_description_index_ = 0;
  }

  if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
    DASH_LOG((BoxName() + " too short").c_str(), "Can not get entry_count",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  if (grouping_type_ != kSeigGroupType) {
    // Only Support SEIG group type.
    return ptr - buffer;
  }

  entry_count_ = ntohlFromBuffer(ptr);
  ptr += sizeof(uint32_t);
  for (uint32_t count = 0; count < entry_count_; ++count) {
    GroupEntry entry;
    if (version_ == 1) {
      if (default_length_ == 0) {
        if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
          DASH_LOG((BoxName() + " too short").c_str(),
                   "Can not get description_length",
                   DumpMemory(buffer, length).c_str());
          return DashParser::kParseFailure;
        }
        description_length_ = ntohlFromBuffer(ptr);
        ptr += sizeof(uint32_t);
      }
    }
    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint32_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(), "Can not get SEIG entry",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    // Reserved 8 bytes, skipping.
    ptr += sizeof(uint8_t);
    uint8_t crypt_and_skip_blocks = ptr[0];
    entry.crypt_byte_block_ = crypt_and_skip_blocks >> 4;
    entry.skip_byte_block_ = crypt_and_skip_blocks & 0xf;
    ptr += sizeof(uint8_t);
    entry.is_protected_ = ptr[0];
    ptr += sizeof(uint8_t);
    entry.per_sample_iv_size_ = ptr[0];
    ptr += sizeof(uint8_t);

    if (!EnoughBytesToParse(ptr - buffer, sizeof(uint16_t), length)) {
      DASH_LOG((BoxName() + " too short").c_str(), "Can not get SEIG entry KID",
               DumpMemory(buffer, length).c_str());
      return DashParser::kParseFailure;
    }
    memcpy(entry.kid_, ptr, kKidSize);
    ptr += kKidSize;

    if (entry.is_protected_ == 1 && entry.per_sample_iv_size_ == 0) {
      entry.constant_iv_.resize(ptr[0]);
      ptr += sizeof(uint8_t);
      if (!EnoughBytesToParse(ptr - buffer, entry.constant_iv_.size(),
                              length)) {
        DASH_LOG((BoxName() + " too short").c_str(), "Data missing.",
                 DumpMemory(buffer, length).c_str());
        return DashParser::kParseFailure;
      }
      memcpy(&entry.constant_iv_[0], ptr, entry.constant_iv_.size());
      ptr += entry.constant_iv_.size();
    }
    entries_.push_back(entry);
  }
  return ptr - buffer;
}

std::string SgpdContents::PrettyPrintEntry(const GroupEntry& entry) const {
  std::string result = "";
  result += " Crypt Byte: " + PrettyPrintValue(entry.crypt_byte_block_);
  result += " Skip Byte: " + PrettyPrintValue(entry.skip_byte_block_);
  result += " Is Protected: " + PrettyPrintValue(entry.is_protected_);
  result +=
      " Per Sample IV Size: " + PrettyPrintValue(entry.per_sample_iv_size_);
  result += " KID: " + PrettyPrintBuffer(entry.kid_, kKidSize);
  result += " Constant IV: " + PrettyPrintBuffer(entry.constant_iv_.data(),
                                                 entry.constant_iv_.size());
  return result;
}

std::string SgpdContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  for (const GroupEntry& entry : entries_) {
    result += "\n" + indent + PrettyPrintEntry(entry);
  }
  return result;
}
}  //  namespace dash2hls
