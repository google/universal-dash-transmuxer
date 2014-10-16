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

#include <gtest/gtest.h>

#include "box_type.h"
#include "utilities.h"

namespace dash2hls {

TEST(DashToHls, BoxType) {
  uint8_t vmhd[4];
  memcpy(vmhd, "vmhd", sizeof(vmhd));
  BoxType box_type_1;
  box_type_1.set_type(BoxType::kBox_vmhd);
  EXPECT_EQ(std::string("vmhd"), box_type_1.PrettyPrint(""));
  EXPECT_EQ(BoxType::kBox_vmhd, box_type_1.asUint32());
  EXPECT_EQ(ntohl(BoxType::kBox_vmhd), box_type_1.asBoxType());

  BoxType box_type_2;
  box_type_2.set_type(vmhd);
  EXPECT_EQ(std::string("vmhd"), box_type_1.PrettyPrint(""));
  EXPECT_EQ(BoxType::kBox_vmhd, box_type_1.asUint32());
  EXPECT_EQ(ntohl(BoxType::kBox_vmhd), box_type_1.asBoxType());
}
}  // namespace dash2hls
