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

#include "library/dash/box_type.h"

#include "library/utilities.h"

namespace dash2hls {

void BoxType::set_type(Type type) {
  box_type_[0] = (type >> 24) & 0xff;
  box_type_[1] = (type >> 16) & 0xff;
  box_type_[2] = (type >> 8) & 0xff;
  box_type_[3] = (type) & 0xff;
}

void BoxType::set_type(const uint8_t* type) {
  box_type_[0] = type[0];
  box_type_[1] = type[1];
  box_type_[2] = type[2];
  box_type_[3] = type[3];
}

uint32_t BoxType::asUint32() const {
  return ntohl(*(reinterpret_cast<const uint32_t *>(box_type_)));
}

uint32_t BoxType::asBoxType() const {
  return *(reinterpret_cast<const uint32_t *>(box_type_));
}

std::string BoxType::PrettyPrint(std::string indent) const {
  return std::string(box_type_, 4);
}

}  // namespace dash2hls
