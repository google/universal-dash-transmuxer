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

// Light weight bit reader.  Does not keep a copy or memory nor is it thread
// safe.  Meant strictly for the simple cases of a routine needing to parse
// a single dash box.  Trying to use it generically or in complicated
// situations is unwise.
//
// Read methods check for overflows and return false if there are not enough
// bits in the buffer or bits_to_read is greater than the size of value.
//
// EXAMPLE:
//   This will read 1 bit into the byte_value then the next 12 bits into
//   the short_value.  If aVector does not have enough bits then it will call
//   Error.
//
//   vector<uint8_t> aVector;
//   uint8_t byte_value = 0;
//   uint16_t short_value = 0;
//   BitReader bit_reader(&aVector[0], aVector.size());
//   if (!bit_reader.read(1, &byte_value)) {
//     Error("Could not read byte_value");
//   }
//   if (!bit_reader.read(12, &short_value)) {
//     Error("Could not read short_value");
//   }

#ifndef DASHTOHLS_BIT_READER_H_
#define DASHTOHLS_BIT_READER_H_

#include <stddef.h>
#include <stdint.h>

namespace dash2hls {
class BitReader {
 public:
  BitReader(const uint8_t* bits, size_t length);
  bool Read(size_t bits_to_read, uint8_t* value);
  bool Read(size_t bits_to_read, uint16_t* value);
  bool Read(size_t bits_to_read, uint32_t* value);
  bool Read(size_t bits_to_read, uint64_t* value);
 private:
  template <typename T> bool ReadInternal(size_t bits_to_read, T* value);

  const uint8_t* bits_;
  size_t length_;
  size_t bit_position_;
  size_t byte_position_;
};  // class BitReader
}  // namespace dash2hls
#endif  // DASHTOHLS_BIT_READER_H_
