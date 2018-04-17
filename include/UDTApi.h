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

#ifndef _UDT_DASH_TRANSMUXER_UDTAPI_H_
#define _UDT_DASH_TRANSMUXER_UDTAPI_H_

// C API for converting DASH content to HLS.
//
// Even though the internal code uses C++ the API exposed is pure C.  This
// eases adoption by iOS and MacOS developers used to Objective-C.
//
// UDT converts a subset of possible DASH content other formats. This API
// exposes the ability to convert DASH to HLS.  The current subset is geared
// towards GooglePlay DASH formats, namely H.264 content in
// fragmented mp4.
//
// Files are required to have a sidx box containing the locations to each
// moof/mdat segment.  The sidx is usually near the beginning of the dash
// file, usually in the first few k, but that is not guaranteed.
//
// Each moof/mdat will be converted into exactly one HLS .ts segment.
// Example is included in ../library/player.
//
// CENC Reference:  http://www.w3.org/TR/encrypted-media/cenc-format.html
// Format Reference: http://goo.gl/vNe3qm
// box  = Field in Header
// mdat = Media Data Boxes
// moof = Movie Fragment
// saiz = SampleAuxiliaryInformationSizesBox
// saio = SampleAuxiliaryInformationOffsetsBox
// tenc = Track Encryption Box
// tfdt = Track Fragment Decode Time Box
// trun = Track Fragment Run Boxes

#include <stdint.h>
#include <unistd.h>

#include "DashToHlsApi.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create the session if there is no error.  |session| memory is owned by
// Udt and freed on ReleaseSession.
DashToHlsStatus Udt_CreateSession(struct DashToHlsSession** session);

// ReleaseSession not only frees the |session| but any memory every allocated
// for this |session|.  All indexes, and ts segments will be freed
// and invalidated.
DashToHlsStatus Udt_ReleaseSession(struct DashToHlsSession* session);

// API designed to handle dash data and produce HLS Transport stream (.ts)
// segments.  The |hls_segment| is owned by the |session| and freed
// when ReleaseHlsSegment or ReleaseSession is called.
DashToHlsStatus Udt_ConvertDash(struct DashToHlsSession* session,
                                uint32_t segment_number,
                                const uint8_t* dash_data,
                                size_t dash_data_size,
                                const uint8_t** segment_out,
                                size_t* segment_out_size);

// Parses a DASH file from the beginning until a sidx box is found.  Once a
// sidx box is found an the DashToHlsSegments will be allocated and stored in
// the index.  All allocated memory is owned by DashToHls and freed when the
// |session| is released.  DashToHls_ParseDash will return
// kDashToHlsStatus_NeedsMoreData if the entire sidx box was not found.
// The next call should continue with the next bytes from DASH content.
DashToHlsStatus Udt_ParseDash(struct DashToHlsSession* session,
                              uint8_t stream_index,
                              uint8_t* dash_data,
                              size_t dash_data_size,
                              uint8_t* pssh,
                              size_t pssh_length,
                              struct DashToHlsIndex** index);

// Optional call to free up some memory without destroying the entire
// |session|.
DashToHlsStatus Udt_ReleaseHlsSegment(struct DashToHlsSession* session,
                                      uint32_t hls_segment_number);

// Common Encryption callbacks.  Common encryption (CENC) at Google is handled
// by a module called the CDM.  Other implementations may use their own DRM
// code to handle the decryption.  This library will call the CENC_PsshHandler
// whenever a pssh box is seen with the entire contents of the box (no header).
//
// Encrypted samples will be passed as just the encrypted portions.  See
// ISO 23001-7 for an explanation of how subsample encryption works.  Or,
// just trust me and AES-128-CTR the entire block passed and don't worry
// about the packing.
typedef DashToHlsStatus (*PsshHandler)(DashToHlsContext context,
                                       const uint8_t* pssh,
                                       size_t pssh_length);
DashToHlsStatus Udt_SetPsshHandler(struct DashToHlsSession* session,
                                   DashToHlsContext context,
                                   CENC_PsshHandler pssh_handler);

// There are two modes for decrypting.  If useSampleEntries are set then the
// entire data block, clear and encrypted, are sent to the callback with the
// samples set to an array of SampleEntrySize SampleEntrys.
// If use_sample_entries is false then the Udt library will concatenate
// all encrypted data, pass just the encrypted data, and reinsert the clear
// content.  If use_sample_entries is false then samples will be nullptr.
typedef DashToHlsStatus (*DecryptionHandler)(DashToHlsContext context,
                                             const uint8_t* encrypted,
                                             uint8_t* clear,
                                             size_t length,
                                             uint8_t* iv,
                                             size_t iv_length,
                                             const uint8_t* key_id,
                                             // key_id is always 16 bytes.
                                             struct SampleEntry*,
                                             size_t sampleEntrySize);
DashToHlsStatus Udt_SetDecryptSample(struct DashToHlsSession* session,
                                     DashToHlsContext context,
                                     CENC_DecryptionHandler decryption_handler,
                                     bool use_sample_entries);

// Calls the optional callback with a human readable output of the dash
// parsing.
void Udt_PrettyPrint(struct DashToHlsSession* session);

#ifdef __cplusplus
};
#endif

#endif  // _UDT_DASH_TRANSMUXER_UDTAPI_H_
