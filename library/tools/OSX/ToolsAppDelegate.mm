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

#include <string>

#import "include/DashToHlsApi.h"
#import "ToolsAppDelegate.h"
#import "ToolsMpdParser.h"

void sConverterCallback(const char* callbackString) {
  [(ToolsAppDelegate*)[[NSApplication sharedApplication] delegate]
       AddToTextView:[NSString stringWithUTF8String:callbackString]];
}

@implementation ToolsAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  [_generateTsButton setEnabled:NO];
  [_liveButton setEnabled:NO];
  [_generateM3u8Button setEnabled:YES];
  SetDiagnosticCallback(sConverterCallback);
}

- (IBAction)SetOutputDirectory:(id)sender {
  NSOpenPanel* openPanel = [NSOpenPanel openPanel];
  [openPanel setAllowsMultipleSelection:NO];
  [openPanel setCanChooseDirectories:YES];
  [openPanel setCanChooseFiles:NO];
  [openPanel setPrompt:@"Output"];
  [openPanel setExtensionHidden:NO];
  [openPanel setCanCreateDirectories:YES];
  if ([openPanel runModal] == NSFileHandlingPanelOKButton) {
    NSLog(@"openPanel %@", [openPanel URL]);
    _outputDirectory = [openPanel URL];
    [_outputDirectoryButton setTitle:[_outputDirectory lastPathComponent]];
    [_generateTsButton setEnabled:_outputDirectory && _mp4Url];
    [_liveButton setEnabled:_outputDirectory && _mp4Url];
  }
}

- (IBAction)GenerateTS:(id)sender {
  NSData* mp4Data = [NSData dataWithContentsOfURL:_mp4Url];
  DashToHlsSession* session = nullptr;
  if (DashToHls_CreateSession(&session) != kDashToHlsStatus_OK) {
    return;
  }
  DashToHlsIndex* index;

  if (DashToHls_ParseDash(session,
                          reinterpret_cast<const uint8_t*>([mp4Data bytes]),
                          [mp4Data length], &index) != kDashToHlsStatus_OK) {
    return;
  }
  for (uint8_t segment = 0; segment < index->index_count; ++segment) {
    DashToHlsStatus status = kDashToHlsStatus_OK;
    const uint8_t* hlsSegment;
    size_t hlsSize;
    status = DashToHls_ConvertDashSegment(session,
                                          segment,
                                          reinterpret_cast<const uint8_t*>([mp4Data bytes]) +
                                              index->segments[segment].location,
                                          index->segments[segment].length,
                                          &hlsSegment,
                                          &hlsSize);
    if (status == kDashToHlsStatus_OK) {
      NSURL* outputFile = [NSURL URLWithString:[NSString stringWithFormat:@"%@/%d.ts",
                                                 [_outputDirectory absoluteString], segment]];
      NSData* hlsData = [NSData dataWithBytes:hlsSegment length:hlsSize];
      [hlsData writeToURL:outputFile atomically:YES];
    } else {
      NSLog(@"Failed to parse segment error %d", status);
    }
  }
  DashToHls_ReleaseSession(session);
}

- (IBAction)Live:(id)sender {
  NSData* mp4Data = [NSData dataWithContentsOfURL:_mp4Url];
  DashToHlsSession* session = nullptr;
  DashToHls_CreateSession(&session);

  const uint8_t* hlsSegment = nullptr;
  size_t hlsSize = 0;
  if (DashToHls_ParseLive(session,
                          reinterpret_cast<const uint8_t*>([mp4Data bytes]),
                          [mp4Data length],
                          0,
                          &hlsSegment,
                          &hlsSize) != kDashToHlsStatus_OK) {
    return;
  }
  NSURL* outputFile = [NSURL URLWithString:[NSString stringWithFormat:@"%@/live.ts",
                                             [_outputDirectory absoluteString]]];
  NSData* hlsData = [NSData dataWithBytes:hlsSegment length:hlsSize];
  [hlsData writeToURL:outputFile atomically:YES];
  DashToHls_ReleaseSession(session);
}

- (void)AddToTextView:(NSString *)text {
  dispatch_async(dispatch_get_main_queue(), ^{
    NSAttributedString* attr = [[NSAttributedString alloc] initWithString:text];
    [[_logTextView textStorage] appendAttributedString:attr];
    [_logTextView scrollRangeToVisible:NSMakeRange([[_logTextView string] length], 0)];
  });
}

- (void)ParseDash {
  NSData* mp4Data = [NSData dataWithContentsOfURL:_mp4Url];
  DashToHlsSession* session = nullptr;
  DashToHls_CreateSession(&session);
  DashToHlsIndex* index;

  DashToHls_ParseDash(session,
                      reinterpret_cast<const uint8_t*>([mp4Data bytes]),
                      [mp4Data length], &index);
  DashToHls_PrettyPrint(session);
  DashToHls_ReleaseSession(session);
}

- (IBAction)SelectDashFile:(id)sender {
  NSOpenPanel* openPanel = [NSOpenPanel openPanel];
  [openPanel setAllowsMultipleSelection:NO];
  [openPanel setCanChooseDirectories:NO];
  [openPanel setCanChooseFiles:YES];
  [openPanel setPrompt:@"DASH file to analyze"];
  [openPanel setExtensionHidden:NO];
  [openPanel setCanCreateDirectories:YES];
  if ([openPanel runModal] == NSFileHandlingPanelOKButton) {
    _mp4Url = [openPanel URL];
    [_generateTsButton setEnabled:_outputDirectory && _mp4Url];
    [_liveButton setEnabled:_outputDirectory && _mp4Url];
    [self ParseDash];
  }
}

- (IBAction)GenerateM3u8:(id)sender {
  NSOpenPanel* openPanel = [NSOpenPanel openPanel];
  [openPanel setAllowsMultipleSelection:NO];
  [openPanel setCanChooseDirectories:NO];
  [openPanel setCanChooseFiles:YES];
  [openPanel setPrompt:@"Mpd"];
  [openPanel setExtensionHidden:NO];
  [openPanel setCanCreateDirectories:YES];
  if ([openPanel runModal] == NSFileHandlingPanelOKButton) {
    ToolsMpdParser *parser = [[ToolsMpdParser alloc] initWithURL:[openPanel URL]];
    [self AddToTextView:[[NSString alloc] initWithData:parser.audioTrack
                                              encoding:NSUTF8StringEncoding]];
    [self AddToTextView:[[NSString alloc] initWithData:parser.videoTrack
                                              encoding:NSUTF8StringEncoding]];
  }
}

@end
