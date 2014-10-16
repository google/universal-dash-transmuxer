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

// This is Sample code.  It does not have a lot of error checking for
// simplicity.  In practice error codes should be checked and handled.

#import "DashHTTPConnection.h"

#import <Responses/HTTPDataResponse.h>
#include <vector>
#include <openssl/aes.h>

#include "include/DashToHlsApi.h"
#include "library/mac_test_files.h"

@implementation DashHTTPConnection

static NSString* kVariantPlaylist = @"#EXTM3U\n#EXT-X-MEDIA:URI=\"http://"
    @"localhost:12345/audio.m3u8\",TYPE=AUDIO,GROUP-ID=\"audio\",NAME=\""
    @"audio\",DEFAULT=YES,AUTOSELECT=YES\n#EXT-X-STREAM-INF:BANDWIDTH=340992,"
    @"CODECS=\"avc1.4d4015,mp4a.40.5\",RESOLUTION=426x240,AUDIO=\"audio\"\n"
    @"http://localhost:12345/video.m3u8";

static NSString* kPlaylist = @"#EXTM3U\n#EXT-X-VERSION:3\n"
    @"#EXT-X-PLAYLIST-TYPE:VOD\n#EXT-X-TARGETDURATION:7\n%@#EXT-X-ENDLIST";

static NSString* kVideoSegmentFormat = @"#EXTINF:%0.06f,\nhttp://localhost:"
    @"12345/video%d.ts\n";
static NSString* kAudioSegmentFormat = @"#EXTINF:%0.06f,\nhttp://localhost:"
    @"12345/audio%d.ts\n";

static size_t kDashHeaderRead = 10000;  // Enough to get the sidx.
static float k90KRatio = 90000.0;
static const uint8_t video_key[] = {0x6f, 0xc9, 0x6f, 0xe6, 0x28, 0xa2, 0x65, 0xb1,
  0x3a, 0xed, 0xde, 0xc0, 0xbc, 0x42, 0x1f, 0x4d};
static const uint8_t audio_key[] = {0xab, 0x88, 0xf8, 0xd3, 0xe9, 0xea, 0x83, 0x5a,
  0x74, 0x50, 0x53, 0xd7, 0xf4, 0x4d, 0x14, 0x3d};

// CTR is symetrical.  There is no decrypt call, you just use encrypt.
DashToHlsStatus VideoDecryptionHandler(void* context,
                                       const uint8_t* encrypted,
                                       uint8_t* clear,
                                       size_t length,
                                       uint8_t* iv,
                                       size_t iv_length,
                                       const uint8_t* key_id,
                                       struct SampleEntry*,
                                       size_t sampleEntrySize) {
  AES_KEY aes_key;
  AES_set_encrypt_key(video_key, AES_BLOCK_SIZE * 8, &aes_key);
  uint8_t ecount_buf[AES_BLOCK_SIZE];
  memset(ecount_buf, 0, AES_BLOCK_SIZE);
  unsigned int block_offset_cur = 0;
  AES_ctr128_encrypt(encrypted, clear, length, &aes_key, iv, ecount_buf,
                     &block_offset_cur);
  return kDashToHlsStatus_OK;
}

DashToHlsStatus AudioDecryptionHandler(void* context,
                                       const uint8_t* encrypted,
                                       uint8_t* clear,
                                       size_t length,
                                       uint8_t* iv,
                                       size_t iv_length,
                                       const uint8_t* key_id,
                                       struct SampleEntry*,
                                       size_t sampleEntrySize) {
  AES_KEY aes_key;
  AES_set_encrypt_key(audio_key, AES_BLOCK_SIZE * 8, &aes_key);
  uint8_t ecount_buf[AES_BLOCK_SIZE];
  memset(ecount_buf, 0, AES_BLOCK_SIZE);
  unsigned int block_offset_cur = 0;
  AES_ctr128_encrypt(encrypted, clear, length, &aes_key, iv, ecount_buf,
                     &block_offset_cur);
  return kDashToHlsStatus_OK;
}

DashToHlsStatus PsshHandler(void* context,
                            const uint8_t* pssh,
                            size_t pssh_length) {
  return kDashToHlsStatus_OK;
}


// One session with one audio and one video track.
// Required ParseVideo and ParseAudio to be called before any Convert methods.
//
// In theory the constructor could just do that but this is an example and
// shows how to handle getting the m3u8.
class DashParserInfo {
public:
  DashParserInfo() : video_index_(nil), audio_index_(nil){
    DashToHls_CreateSession(&video_);
    DashToHls_CreateSession(&audio_);
    DashToHls_SetCenc_PsshHandler(video_, nullptr, PsshHandler);
    DashToHls_SetCenc_DecryptSample(audio_, nullptr, AudioDecryptionHandler,
                                    false);
    DashToHls_SetCenc_PsshHandler(audio_, nullptr, PsshHandler);
    DashToHls_SetCenc_DecryptSample(video_, nullptr, VideoDecryptionHandler,
                                    false);
    video_file_ = Dash2HLS_GetTestCencVideoFile();
    fseek(video_file_, 0, SEEK_END);
    video_file_size_ = ftell(video_file_);
    fseek(video_file_, 0, SEEK_SET);
    audio_file_ = Dash2HLS_GetTestCencAudioFile();
    fseek(audio_file_, 0, SEEK_END);
    audio_file_size_ = ftell(audio_file_);
    fseek(audio_file_, 0, SEEK_SET);
  }

  ~DashParserInfo() {
    DashToHls_ReleaseSession(video_);
    DashToHls_ReleaseSession(audio_);
    fclose(video_file_);
    fclose(audio_file_);
  }

  // The client has asked for the video manifest, "download" the file and
  // parse enough of the dash to get the sidx.
  void ParseVideo() {
    size_t bytes_read = 0;
    uint8_t buffer[kDashHeaderRead];
    fseek(video_file_, 0, SEEK_SET);
    bytes_read = fread(buffer, 1, kDashHeaderRead, video_file_);
    DashToHls_ParseDash(video_, buffer, bytes_read, &video_index_);
  }

  // The client has asked for the audio manifest, "download" the file and
  // parse enough of the dash to get the sidx.
  void ParseAudio() {
    size_t bytes_read = 0;
    uint8_t buffer[kDashHeaderRead];
    fseek(audio_file_, 0, SEEK_SET);
    bytes_read = fread(buffer, 1, kDashHeaderRead, audio_file_);
    DashToHls_ParseDash(audio_, buffer, bytes_read, &audio_index_);
  }

  // The client has asked for a video segment.  "download" it, convert it
  // to HLS.
  NSData* ConvertVideoSegment(uint32_t segment) {
    size_t dash_buffer_size = video_index_->segments[segment].length;
    size_t bytes_read = 0;
    uint8_t *buffer = new uint8_t[dash_buffer_size]  ;
    fseek(video_file_, video_index_->segments[segment].location, SEEK_SET);
    bytes_read = fread(buffer, 1, video_index_->segments[segment].length,
                       video_file_);
    if (bytes_read != video_index_->segments[segment].length) {
      return nil;
    }
    const uint8_t* hls_segment;
    size_t hls_size;
    DashToHlsStatus status = kDashToHlsStatus_OK;
    status = DashToHls_ConvertDashSegment(video_, segment, buffer,
                                          dash_buffer_size, &hls_segment,
                                          &hls_size);
    delete[] buffer;
    if (status == kDashToHlsStatus_OK) {
      NSData* data = [NSData dataWithBytes:hls_segment length:hls_size];
      DashToHls_ReleaseHlsSegment(video_, segment);
      return data;
    }
    return nil;
  }

  // The client has asked for an audio segment.  "download" it, convert it
  // to HLS.
  NSData* ConvertAudioSegment(uint32_t segment) {
    size_t dash_buffer_size = audio_index_->segments[segment].length;
    size_t bytes_read = 0;
    uint8_t *buffer = new uint8_t[dash_buffer_size]  ;
    fseek(audio_file_, audio_index_->segments[segment].location, SEEK_SET);
    bytes_read = fread(buffer, 1, audio_index_->segments[segment].length,
                       audio_file_);
    if (bytes_read != audio_index_->segments[segment].length) {
      return nil;
    }
    const uint8_t* hls_segment;
    size_t hls_size;
    DashToHlsStatus status = kDashToHlsStatus_OK;
    status = DashToHls_ConvertDashSegment(audio_, segment, buffer,
                                          dash_buffer_size, &hls_segment,
                                          &hls_size);
    delete[] buffer;
    if (status == kDashToHlsStatus_OK) {
      NSData* data = [NSData dataWithBytes:hls_segment length:hls_size];
      DashToHls_ReleaseHlsSegment(audio_, segment);
      return data;
    }
    return nil;
  }

  const DashToHlsSession* get_video() const {return video_;}
  const DashToHlsIndex* get_video_index() const {return video_index_;}
  const DashToHlsSession* get_audio() const {return video_;}
  const DashToHlsIndex* get_audio_index() const {return audio_index_;}

  // There can be multiple DashHTTPConnections as the the client can download
  // multiple segments at once.  In practice it is up to the client to
  // handle matching HTTPConnections to the DashToHlsSessions.
  static DashParserInfo* s_info;

  size_t get_video_file_size() {return video_file_size_;}
  size_t get_audio_file_size() {return audio_file_size_;}

private:
  DashToHlsSession* video_;
  DashToHlsIndex* video_index_;
  FILE* video_file_;
  size_t video_file_size_;
  DashToHlsSession* audio_;
  DashToHlsIndex* audio_index_;
  FILE* audio_file_;
  size_t audio_file_size_;
};

DashParserInfo* DashParserInfo::s_info = nullptr;

- (id)initWithAsyncSocket:(GCDAsyncSocket *)newSocket
            configuration:(HTTPConfig *)aConfig {
  self = [super initWithAsyncSocket:newSocket configuration:aConfig];
  if (!DashParserInfo::s_info) {
    DashParserInfo::s_info = new DashParserInfo;
  }
  return self;
}

// Build a simple playlist for either the video or audio.
- (NSString*)buildManifest:(BOOL)is_video {
  if (is_video) {
    if (DashParserInfo::s_info->get_video_index() == nil) {
      DashParserInfo::s_info->ParseVideo();
    }
  } else {
    if (DashParserInfo::s_info->get_audio_index() == nil) {
      DashParserInfo::s_info->ParseAudio();
    }
  }

  NSMutableString* segments = [NSMutableString string];
  const DashToHlsIndex* index = NULL;
  if (is_video) {
    index = DashParserInfo::s_info->get_video_index();
  } else {
    index = DashParserInfo::s_info->get_audio_index();
  }
  uint32_t max_duration = 0;
  // The sample used here is truncated.  iOS will not play a truncated
  // asset.  It just silently fails.
  size_t file_size = is_video ? DashParserInfo::s_info->get_video_file_size() :
      DashParserInfo::s_info->get_audio_file_size();
  for (size_t count = 0; count < index->index_count; ++count) {
    if (index->segments[count].location + index->segments[count].length >
        file_size) {
      break;
    }
    if (index->segments[count].duration > max_duration) {
      max_duration = index->segments[count].duration;
    }
    [segments appendFormat:is_video ? kVideoSegmentFormat:kAudioSegmentFormat,
     index->segments[count].duration / k90KRatio, count, NULL];
  }
  return [NSString stringWithFormat:kPlaylist, segments, NULL];
}

// Handle the requests from the client.  The manifest is written to be as
// easy to parse as possible.
- (NSObject<HTTPResponse> *)httpResponseForMethod:(NSString *)method
                                              URI:(NSString *)path {
  NSLog(@"called with %@ %@", method, path);
  NSData* page_data = NULL;
  if ([path isEqualToString:@"/movie.m3u8"]) {
    page_data = [kVariantPlaylist dataUsingEncoding:NSUTF8StringEncoding];
  } else if ([path isEqualToString:@"/video.m3u8"]) {
    page_data = [[self buildManifest:YES]
                 dataUsingEncoding:NSUTF8StringEncoding];
  } else if ([path isEqualToString:@"/audio.m3u8"]) {
    page_data= [[self buildManifest:NO]
        dataUsingEncoding:NSUTF8StringEncoding];
  } else {
    NSScanner* scanner = [NSScanner scannerWithString:path];
    if ([scanner scanString:@"/video" intoString:NULL]) {
      int segment;
      if ([scanner scanInt:&segment]) {
        NSLog(@"Found video segment %d", segment);
      } else {
        NSLog(@"found video tag, no segment scanned");
      }
      page_data = DashParserInfo::s_info->ConvertVideoSegment(segment);
    } else if ([scanner scanString:@"/audio" intoString:NULL]) {
      int segment;
      if ([scanner scanInt:&segment]) {
        NSLog(@"Found audio segment %d", segment);
      } else {
        NSLog(@"found audio tag, no segment scanned");
      }
      page_data = DashParserInfo::s_info->ConvertAudioSegment(segment);
    }
  }
  if (page_data) {
    return [[HTTPDataResponse alloc] initWithData:page_data];
  }
  return nil;
}

@end
