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

#include "library/dash/tenc_contents.h"

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"
#include "library/utilities.h"

namespace dash2hls {

// See ISO 23001-7 for details.
// aligned(8) class TrackEncryptionBox extends FullBox(‘tenc’, version=0,
//                                                     flags=0)
// {
//   unsigned int(24) default_IsEncrypted;
//   unsigned int(8) default_IV_size;
//   unsigned int(8)[16] default_KID;
// }
size_t TencContents::Parse(const uint8_t* buffer, size_t length) {
  const uint8_t* ptr = buffer + FullBoxContents::Parse(buffer, length);
  if ((ptr == buffer) ||
      !EnoughBytesToParse(ptr - buffer,
                          kDefaultIsEncryptedSize + sizeof(uint8_t) +
                          kKidSize, length)) {
    DASH_LOG((BoxName() + " too short").c_str(),
             "At least 20 bytes are required",
             DumpMemory(buffer, length).c_str());
    return DashParser::kParseFailure;
  }
  default_is_encrypted_ = ntohlFromBuffer(ptr) >> kDefaultShift;
  ptr += kDefaultIsEncryptedSize;
  default_iv_size_ = *ptr;
  ++ptr;
  memcpy(default_kid_, ptr, kKidSize);
  ptr += kKidSize;
  return ptr - buffer;
}

std::string TencContents::PrettyPrint(std::string indent) const {
  std::string result = FullBoxContents::PrettyPrint(indent);
  result += " IsEncrypted: " + PrettyPrintValue(default_is_encrypted_);
  result += " IV size:" + PrettyPrintValue(default_iv_size_);
  result += " Default KID:" + PrettyPrintBuffer(default_kid_, kKidSize);
  return result;
}
}  // namespace dash2hls
