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

// Creates an MPMoviePlayer and starts playing the sample movie.
//
// Uses MPMoviePlayer because iOS does not have an AVPlayerView, so using
// the AV framework on iOS adds unrelated code to get an AVPlayer to show
// up on the screen.
#import <UIKit/UIKit.h>

@interface ViewController : UIViewController
@end
