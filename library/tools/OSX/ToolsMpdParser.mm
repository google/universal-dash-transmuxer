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

#import "ToolsMpdParser.h"

static NSString* kVariantPlaylist =
    @"#EXTM3U\n#EXT-X-MEDIA:URI=\"http://"
    @"localhost:12345/audio.m3u8\",TYPE=AUDIO,GROUP-ID=\"audio\",NAME=\""
    @"audio\",DEFAULT=YES,AUTOSELECT=YES\n#EXT-X-STREAM-INF:BANDWIDTH=340992,"
    @"CODECS=\"avc1.4d4015,mp4a.40.5\",RESOLUTION=426x240,AUDIO=\"audio\"\n"
    @"http://localhost:12345/video.m3u8";

static NSString* kPlaylist =
    @"#EXTM3U\n#EXT-X-VERSION:3\n"
    @"#EXT-X-MEDIA-SEQUENCE:%d\n"
    @"#EXT-X-PLAYLIST-TYPE:VOD\n#EXT-X-TARGETDURATION:%0.06f\n";

static NSString* kVideoSegmentFormat = @"#EXTINF:%0.06f,\n%@\n";
static NSString* kAudioSegmentFormat = @"#EXTINF:%0.06f,\n%@\n";

static NSString* kXPathToSegments = @"/MPD[1]/Period[1]/SegmentList[1]";
static NSString* kXPathToDurations = @"/MPD[1]/Period[1]/SegmentList[1]/SegmentTimeline[1]/S";
static NSString* kXPathToAudio =
    @"/MPD[1]/Period[1]/AdaptationSet[1]/Representation[1]/SegmentList";
static NSString* kXPathToVideo =
    @"/MPD[1]/Period[1]/AdaptationSet[2]/Representation[1]/SegmentList";

@implementation ToolsMpdParser {
  NSXMLDocument* _xmlDocument;
}

- (id)initWithURL:(NSURL *)url {
  self = [super init];
  if (self) {
    NSError* error;
    _xmlDocument = [[NSXMLDocument alloc] initWithContentsOfURL:url options:0 error:&error];

    // We know the MPD YouTube uses so we are going to assume it isn't fragile.
    // In practice you should write non-fragile code with Xpath.
    NSArray* segmentListArray =
        [[_xmlDocument rootElement] nodesForXPath:kXPathToSegments error:&error];
    NSXMLElement* segmentList = segmentListArray[0];
    size_t startNumber =
        [[[segmentList attributeForName:@"startNumber"] objectValue] integerValue];
    size_t timescale =
        [[[segmentList attributeForName:@"timescale"] objectValue] integerValue];
    NSArray* durationsXml =
        [[_xmlDocument rootElement] nodesForXPath:kXPathToDurations error:&error];
    NSMutableArray* durations = [NSMutableArray array];
    float maxDuration = 0;
    for (NSXMLElement *element in durationsXml) {
      size_t trackDuration =[[[element attributeForName:@"d"] objectValue] intValue];
      [durations addObject:[NSNumber numberWithLongLong:trackDuration]];
      if (trackDuration > maxDuration) {
        maxDuration = trackDuration;
      }
    }

    NSMutableString* m3u8Audio =
        [NSMutableString stringWithFormat:kPlaylist, startNumber, maxDuration / timescale, nil];

    NSXMLElement* audioTrackRoot =
        [[_xmlDocument rootElement] nodesForXPath:kXPathToAudio error:&error][0];
    NSArray* audioElements = [audioTrackRoot elementsForName:@"SegmentURL"];
    for (size_t count = 0; count < [audioElements count]; ++count) {
      float duration = [durations[count] floatValue];
      [m3u8Audio appendString:
        [NSString stringWithFormat:kVideoSegmentFormat, duration / timescale,
            [[audioElements[count] attributeForName:@"media"] objectValue]]];
    }

    _audioInitialization =
        [[[audioTrackRoot elementsForName:@"Initialization"][0] attributeForName:@"sourceURL"]
             objectValue];
    _audioInitializationRange =
        [[[audioTrackRoot elementsForName:@"Initialization"][0]
             attributeForName:@"range"] objectValue];

    NSMutableString* m3u8Video = [NSMutableString stringWithFormat:kPlaylist, startNumber,
                                  maxDuration / timescale, NULL];

    NSXMLElement* videoTrackRoot =
        [[_xmlDocument rootElement] nodesForXPath:kXPathToVideo error:&error][0];
    NSArray* videoElements = [videoTrackRoot elementsForName:@"SegmentURL"];
    for (size_t count = 0; count < [videoElements count]; ++count) {
      float duration = [durations[count] floatValue];
      [m3u8Video appendString:
          [NSString stringWithFormat:kVideoSegmentFormat, duration / timescale,
              [[videoElements[count] attributeForName:@"media"] objectValue]]];
    }

    _videoInitialization =
        [[[videoTrackRoot elementsForName:@"Initialization"][0] attributeForName:@"sourceURL"]
            objectValue];
    _videoInitializationRange =
        [[[videoTrackRoot elementsForName:@"Initialization"][0] attributeForName:@"range"]
            objectValue];

    _audioTrack = [m3u8Audio dataUsingEncoding:NSUTF8StringEncoding];
    _videoTrack = [m3u8Video dataUsingEncoding:NSUTF8StringEncoding];
  }
  return self;
}

@end
