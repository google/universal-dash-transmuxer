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

#import <Foundation/Foundation.h>

namespace {
NSString* kMp4BoxInitSegment = @"mp4box_init";
NSString* kDashSampleCencVideoFile = @"sintel-cenc-video";
NSString* kDashSampleCencAudioFile = @"sintel-cenc-audio";
NSString* kDashSampleVideoFile = @"dash-160";
NSString* kDashSampleAudioFile = @"dash-139";
NSString* kDashSampleVideoHeader = @"sintel-cenc-video-header";
NSString* kDashSampleVideoSegment = @"sintel-cenc-video-segment3";
}  // namespace

FILE* Dash2HLS_GetTestVideoFile() {
  NSString* mp4_path =
      [[NSBundle mainBundle] pathForResource:kDashSampleVideoFile
           ofType:@"fmp4"];
  return fopen([mp4_path UTF8String], "r");
}

FILE* Dash2HLS_GetTestAudioFile() {
  NSString* mp4_path =
      [[NSBundle mainBundle] pathForResource:kDashSampleAudioFile
          ofType:@"fmp4"];
  return fopen([mp4_path UTF8String], "r");
}

FILE* Dash2HLS_GetTestCencVideoFile() {
  NSString* mp4_path =
      [[NSBundle mainBundle] pathForResource:kDashSampleCencVideoFile
           ofType:@"mp4"];
  return fopen([mp4_path UTF8String], "r");
}

FILE* Dash2HLS_GetTestCencAudioFile() {
  NSString* mp4_path =
      [[NSBundle mainBundle] pathForResource:kDashSampleCencAudioFile
          ofType:@"mp4"];
  return fopen([mp4_path UTF8String], "r");
}

FILE* Dash2HLS_GetTestCencVideoHeader() {
  NSString* mp4_path =
      [[NSBundle mainBundle] pathForResource:kDashSampleVideoHeader
           ofType:@"mp4"];
  return fopen([mp4_path UTF8String], "r");
}

FILE* Dash2HLS_GetTestCencVideoSegment() {
  NSString* mp4_path =
      [[NSBundle mainBundle] pathForResource:kDashSampleVideoSegment
           ofType:@"mp4"];
  return fopen([mp4_path UTF8String], "r");
}

FILE* Dash2HLS_GetMp4BoxInitSegment() {
  NSString* mp4_path =
      [[NSBundle mainBundle] pathForResource:kMp4BoxInitSegment
                                      ofType:@"mp4"];
  return fopen([mp4_path UTF8String], "r");
}
