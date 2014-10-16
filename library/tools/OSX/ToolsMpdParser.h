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

// ToolsMpdParser is a standalone class to take a standard
// YouTube or gFiber MPD file and turn it into a set of
// m3u8 files.  It is expected to be used when converting
// DASH to HLS.

#import <Foundation/Foundation.h>

@interface ToolsMpdParser : NSObject<NSXMLParserDelegate>

// _audioTrack contains the audio m3u8.
@property(nonatomic, readonly) NSData *audioTrack;

// URL and range for getting the DASH header.  Pass this data
// to the DashToHlsApi.
@property(nonatomic, readonly) NSString *audioInitialization;
@property(nonatomic, readonly) NSString *audioInitializationRange;

// _videoTrack contains the video m3u8.
@property(nonatomic, readonly) NSData *videoTrack;

// URL and range for getting the DASH header.  Pass this data
// to the DashToHlsApi.
@property(nonatomic, readonly) NSString *videoInitialization;
@property(nonatomic, readonly) NSString *videoInitializationRange;

- (id)initWithURL:(NSURL *)url;

@end
