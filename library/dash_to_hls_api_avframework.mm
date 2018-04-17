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

#include "CommonCrypto/CommonCrypto.h"
#include "Security/Security.h"

#include "include/DashToHlsApiAVFramework.h"
#include "library/dash_to_hls_session.h"

namespace {
  const size_t kKeySize = 16;
  std::vector<uint8_t> s_key;
  std::vector<uint8_t> s_iv;
  // Weak maps AVURLAsset to a strong DashToHLSResourceLoader.  This prevents the
  // DashToHLSResourceLoader from being immediately deallocated.
  NSMapTable* s_DelegateMap;
}

static NSString* kSchemeName = @"wvkey";

@interface DashToHLSResourceLoader : NSObject<AVAssetResourceLoaderDelegate>
@property(readonly, strong) id<AVAssetResourceLoaderDelegate> avDelegate;
@property(readonly, weak) AVURLAsset* originalAsset;

- (DashToHLSResourceLoader*)initWithDelegate:(id<AVAssetResourceLoaderDelegate>)delegate
                                       asset:(AVURLAsset*)asset;
@end

@implementation DashToHLSResourceLoader

- (DashToHLSResourceLoader*)initWithDelegate:(id<AVAssetResourceLoaderDelegate>)delegate
                                       asset:(AVURLAsset *)asset {
  self = [super init];
  if (self) {
    _avDelegate = delegate;
    _originalAsset = asset;
  }
  return self;
}

// We want to give the key only when asked by a registered AVURLAsset and
// there was no obvious spoofing of classes.  Arxan should protect the key
// from anything swizzling.
- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader
    shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest {
  if (([loadingRequest.request.URL.scheme isEqualToString:kSchemeName]) &&
      ([resourceLoader isMemberOfClass:[AVAssetResourceLoader class]]) &&
      ([_originalAsset resourceLoader] == resourceLoader)) {
    [loadingRequest.dataRequest respondWithData:[NSData dataWithBytes:s_key.data()
                                                               length:s_key.size()]];
    [loadingRequest finishLoading];
    return YES;
  }
  if ([_avDelegate respondsToSelector:
           @selector(resourceLoader:shouldWaitForLoadingOfRequestedResource:)]) {
    return [_avDelegate resourceLoader:resourceLoader
                shouldWaitForLoadingOfRequestedResource:loadingRequest];
  } else {
    return NO;
  }
}

- (void)resourceLoader:(AVAssetResourceLoader *)resourceLoader
    didCancelLoadingRequest:(AVAssetResourceLoadingRequest *)loadingRequest {
  if ([_avDelegate respondsToSelector:@selector(resourceLoader:didCancelLoadingRequest:)]) {
    [_avDelegate resourceLoader:resourceLoader didCancelLoadingRequest:loadingRequest];
  }
}

- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader
    shouldWaitForRenewalOfRequestedResource:(AVAssetResourceRenewalRequest *)renewalRequest {
  if ([_avDelegate respondsToSelector:
           @selector(resourceLoader:shouldWaitForRenewalOfRequestedResource:)]) {
    return [_avDelegate resourceLoader:resourceLoader
                shouldWaitForRenewalOfRequestedResource:renewalRequest];
  } else {
    return NO;
  }
}

@end

using dash2hls::Session;

void SetEncryptOutputFlag(struct DashToHlsSession* session) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  dash_session->encrypt_output_ = true;
}

// Session can be NULL, but the session will not know that encryption is ready.
void Udt_InitializeEncryption(struct DashToHlsSession* session) {
  static dispatch_once_t once;
  if (session) {
    SetEncryptOutputFlag(session);
  }
  dispatch_once(&once, ^{
    s_key.resize(kKeySize);
    s_iv.resize(kKeySize);
    if ((SecRandomCopyBytes(NULL, kKeySize, s_key.data()) != 0) ||
        (SecRandomCopyBytes(NULL, kKeySize, s_iv.data()) != 0)) {
      s_key.clear();
      s_iv.clear();
    }
  });
}

DashToHlsStatus Udt_SetAVURLAsset(AVURLAsset* asset,
                                        id<AVAssetResourceLoaderDelegate> assetDelegate,
                                        dispatch_queue_t queue) {
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    s_DelegateMap = [NSMapTable weakToStrongObjectsMapTable];
  });
  if ([asset isMemberOfClass:[AVURLAsset class]]) {
    DashToHLSResourceLoader* udtDelegate =
        [[DashToHLSResourceLoader alloc] initWithDelegate:assetDelegate asset:asset];
    [s_DelegateMap setObject:udtDelegate forKey:asset];

    [[asset resourceLoader] setDelegate:udtDelegate queue:queue];
  }
  return kDashToHlsStatus_OK;
}

// |session| is currently used for detecting encryption. In the future,
// it will give the flexibility to use different keys for each session.
NSString* GetKeyUrl(struct DashToHlsSession* session) {
  if (s_key.empty()) {
    Udt_InitializeEncryption(session);
    if (s_key.empty()) {
      return nil;
    }
  }
  return [NSString stringWithFormat:@"#EXT-X-KEY:METHOD=AES-128,URI=\"%@"
              @"://key.bin\",IV=0x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
              @"%02x%02x%02x%02x%02x%02x%02x\n", kSchemeName,
              s_iv[0], s_iv[1], s_iv[2], s_iv[3], s_iv[4], s_iv[5], s_iv[6], s_iv[7],
              s_iv[8], s_iv[9], s_iv[10], s_iv[11], s_iv[12], s_iv[13], s_iv[14], s_iv[15]];
}

namespace dash2hls {

bool Encrypt(const DashToHlsSession* session, std::vector<uint8_t>* block) {
  if (s_key.empty()) {
    NSLog(@"Udt_InitializeEncryption must be called before Encrypt");
    return false;
  }

  std::vector<uint8_t> encrypted_data;
  size_t encrypted_length = block->size() + kKeySize;
  encrypted_data.resize(encrypted_length);
  CCCryptorStatus status = CCCrypt(kCCEncrypt,
                                   kCCAlgorithmAES128,
                                   kCCOptionPKCS7Padding,
                                   s_key.data(),
                                   s_key.size(),
                                   s_iv.data(),
                                   block->data(),
                                   block->size(),
                                   encrypted_data.data(),
                                   encrypted_length,
                                   &encrypted_length);
  encrypted_data.resize(encrypted_length);
  block->resize(encrypted_length);
  memcpy(block->data(), encrypted_data.data(), block->size());
  return status == kCCSuccess;
}
}  // namespace dash2hls
