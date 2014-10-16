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

#import "ViewController.h"

#import <AVFoundation/AVFoundation.h>
#import <MediaPlayer/MPMoviePlayerController.h>

NSString* kMovieUrl = @"http://localhost:12345/movie.m3u8";

@interface ViewController ()
@property(strong, nonatomic) IBOutlet UIView* movie_holder;
@property(strong, nonatomic) MPMoviePlayerController* movie_player;
@end

@implementation ViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.movie_player =
      [[MPMoviePlayerController alloc] initWithContentURL:
           [NSURL URLWithString:kMovieUrl]];
  [self.movie_holder addSubview:self.movie_player.view];
  [self.movie_player play];
}

- (void)viewWillLayoutSubviews {
  [super viewWillLayoutSubviews];
  [self.movie_player.view setFrame:[self.movie_holder bounds]];
}

- (void)dealloc {
}

@end
