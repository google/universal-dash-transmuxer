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

#import <Cocoa/Cocoa.h>

@interface ToolsAppDelegate : NSObject <NSApplicationDelegate> {
  NSURL* _outputDirectory;
  NSURL* _mp4Url;
}

- (void)AddToTextView:(NSString*)text;

- (IBAction)SetOutputDirectory:(id)sender;
- (IBAction)Live:(id)sender;
- (IBAction)GenerateTS:(id)sender;
- (IBAction)SelectDashFile:(id)sender;
- (IBAction)GenerateM3u8:(id)sender;

@property(assign) IBOutlet NSWindow *window;
@property(weak) IBOutlet NSButton *outputDirectoryButton;
@property(weak) IBOutlet NSButton *selectDashButton;
@property(weak) IBOutlet NSButton *generateTsButton;
@property(weak) IBOutlet NSButton *liveButton;
@property(weak) IBOutlet NSButton *generateM3u8Button;
@property(unsafe_unretained) IBOutlet NSTextView *logTextView;

@end
