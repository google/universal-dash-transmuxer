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
/*
iOS and OSX require extra care to make sure that only the AVPlayer in the
current application has access to the content in the clear.  To solve that
we use the resource loader to serve a random key.

InitializeEncryption calls SecCopyRandomBytes to create random bytes to build
a key on the fly to encrypt the output TS segments. An error will occur if
attempting to encrypt the output without calling InitializeEncryption first.

When building any m3u8 manifests GetKeyUrl returns the m3u8 text that must be
placed in the m3u8 as is.  It has the full line of text describing the key
and url with IV encoded.

Finally DashToHls_SetAVURLAsset needs to be called on all AVURLAssets need to
set up the AVResourceLoaderDelegate.  An optional
AVAssetResourceLoaderDelegate can be passed in as |assetDelegate|.  This will
be called on all AVAssetResourceLoaderDelegate not handled by the UDT.
*/

#ifndef _UDT_DASH_TRANSMUXER_DASHTOHLSAPIAVFRAMEWORK_H_
#define _UDT_DASH_TRANSMUXER_DASHTOHLSAPIAVFRAMEWORK_H_

#import <AVFoundation/AVFoundation.h>

#include "DashToHlsApi.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void Udt_InitializeEncryption(struct DashToHlsSession* session);

DashToHlsStatus Udt_SetAVURLAsset(AVURLAsset* asset,
                                  id<AVAssetResourceLoaderDelegate> assetDelegate,
                                  dispatch_queue_t queue);
NSString* GetKeyUrl(struct DashToHlsSession* session);
#ifdef __cplusplus
};
#endif  // __cplusplus
#endif  // _UDT_DASH_TRANSMUXER_DASHTOHLSAPIAVFRAMEWORK_H_
