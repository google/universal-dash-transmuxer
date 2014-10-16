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

// Starts a proxy server, sets the server to have DashHTTPConnection handle
// all incoming connections.  Creates an AVPlayerView and starts the sample
// movie.

#import <Cocoa/Cocoa.h>
#import <AVKit/AVKit.h>

@class HTTPServer;

@interface AppDelegate : NSObject <NSApplicationDelegate> {
  HTTPServer* _http_server;
}

@property(assign) IBOutlet AVPlayerView *movie_player;
@property(assign) IBOutlet NSWindow *window;

@end
