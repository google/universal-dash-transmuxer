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

#include "mac_test_files.h"

#include <string>

namespace {
const char *kMp4BoxInitSegment = "mp4box_init.mp4";
const char *kDashSampleCencVideoFile = "sintel-cenc-video.mp4";
const char *kDashSampleCencAudioFile = "sintel-cenc-audio.mp4";
const char *kDashSampleVideoFile = "dash-160.fmp4";
const char *kDashSampleAudioFile = "dash-139.fmp4";
const char *kDashSampleVideoHeader = "sintel-cenc-video-header.mp4";
const char *kDashSampleVideoSegment = "sintel-cenc-video-segment3.mp4";
}  // namespace

FILE* Dash2HLS_GetTestVideoFile() {
  std::string path = std::string(DATA_FILES_PATH) + kDashSampleVideoFile;
  return fopen(path.c_str(), "rb");
}

FILE* Dash2HLS_GetTestAudioFile() {
  std::string path = std::string(DATA_FILES_PATH) + kDashSampleAudioFile;
  return fopen(path.c_str(), "rb");
}

FILE* Dash2HLS_GetTestCencVideoFile() {
  std::string path = std::string(DATA_FILES_PATH) + kDashSampleCencVideoFile;
  return fopen(path.c_str(), "rb");
}

FILE* Dash2HLS_GetTestCencAudioFile() {
  std::string path = std::string(DATA_FILES_PATH) + kDashSampleCencAudioFile;
  return fopen(path.c_str(), "rb");
}

FILE* Dash2HLS_GetTestCencVideoHeader() {
  std::string path = std::string(DATA_FILES_PATH) + kDashSampleVideoHeader;
  return fopen(path.c_str(), "rb");
}

FILE* Dash2HLS_GetTestCencVideoSegment() {
  std::string path = std::string(DATA_FILES_PATH) + kDashSampleVideoSegment;
  return fopen(path.c_str(), "rb");
}

FILE* Dash2HLS_GetMp4BoxInitSegment() {
  std::string path = std::string(DATA_FILES_PATH) + kMp4BoxInitSegment;
  return fopen(path.c_str(), "rb");
}
