#ifndef DASH2HLS_PLAYER_DASHHTTPCONNECTION_H_
#define DASH2HLS_PLAYER_DASHHTTPCONNECTION_H_

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

// Proxy connection for handling one movie.
//
// Uses static variables so multiple DashHTTPConnections will use the same
// DashParsers.  To simplify readability of this example very little error
// detection is done.  In production code the error handling must be
// included.
//
// The included sample asset is truncated.  iOS does not like that so this
// example returns only the first 15 seconds.  Productions code should
// return the entire contents.
//
// Absolutely requires ParseAudio/ParseVideo to be called before Convert.
//
// This example uses amazingly trivial manifests.  No adaption, no audio
// only cellular acceptable sessions, no alternate languages.  Just pure
// simple trivial manifest.

#import <Foundation/Foundation.h>
#import "HTTPConnection.h"

@interface DashHTTPConnection : HTTPConnection
@end

#endif  // DASH2HLS_PLAYER_DASHHTTPCONNECTION_H_
