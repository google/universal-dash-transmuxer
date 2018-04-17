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

#ifndef _UDT_DASH_TRANSMUXER_DASHTOHLSAPI_H_
#define _UDT_DASH_TRANSMUXER_DASHTOHLSAPI_H_

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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  kDashToHlsStatus_OK = 0,
  // Parser is unable to complete the operation but can be continued by
  // calling the routine again with more data.
  kDashToHlsStatus_NeedMoreData,
  // Parser did not parse enough data causing an unrecoverable error.  Usually
  // caused by not handling kDashToHlsStatus_NeedMoreData in a prior call.
  kDashToHlsStatus_NotEnoughParsed,
  kDashToHlsStatus_BadDashContents,
  kDashToHlsStatus_BadConfiguration,
  kDashToHlsStatus_ClearContent,
  kDashToHlsStatus_Last
} DashToHlsStatus;

// Opaque structure used internally to track memory usage.
struct DashToHlsSession;

// The location of one moof/mdat location.  The start_offset_ is from the
// beginning of the file.
struct DashToHlsSegment {
  uint64_t start_time;
  uint64_t duration;
  uint32_t timescale;
  uint64_t location;
  uint64_t length;
};

// The list of all moof/mdat locations.  Memory is owned by the
// DashToHlsSession and is freed by DestroySession.
struct DashToHlsIndex {
  uint32_t index_count;
  const struct DashToHlsSegment* segments;
};

// Create the session if there is no error.  |session| memory is owned by
// DashToHls and freed on ReleaseSession.
DashToHlsStatus DashToHls_CreateSession(struct DashToHlsSession** session);

// ReleaseSession not only frees the |session| but any memory every allocated
// for this |session|.  All indexes, and ts segments will be freed
// and invalidated.
DashToHlsStatus DashToHls_ReleaseSession(struct DashToHlsSession* session);

// Parses a DASH file from the beginning until a sidx box is found.  Once a
// sidx box is found an the DashToHlsSegments will be allocated and stored in
// the index.  All allocated memory is owned by DashToHls and freed when the
// |session| is released.  DashToHls_ParseDash will return
// kDashToHlsStatus_NeedsMore if the entire sidx box was not found.  The next
// call should continue with the next bytes from DASH content.
DashToHlsStatus DashToHls_ParseDash(struct DashToHlsSession* session,
                                   const uint8_t* bytes, size_t length,
                                   struct DashToHlsIndex** index);

// Parses a sidx box in |bytes| and |length| and return the
// DashToHlsSegments in |index|.
//
// DashToHlsSegment.location are relative to 0. So you'll need to add the
// startOffset of the sidx.
DashToHlsStatus DashToHls_ParseSidx(struct DashToHlsSession* session,
                                    const uint8_t* bytes,
                                    uint64_t length,
                                    struct DashToHlsIndex** index);

// Returns the |pts| and the |duration| of a dash |bytes|.
DashToHlsStatus DashToHls_ParseSegmentPTS(struct DashToHlsSession* session,
                                          const uint8_t* bytes,
                                          uint64_t length,
                                          uint64_t* pts,
                                          uint64_t* duration);

// The pssh is usually handled out of band.  To simplify things this call
// only extracts a pssh and calls the pssh callback.
//
// pssh requests should be cached, so when DashToHls_ParseLive is called
// the pssh handler will be expected to ignore the request as already
// completed.
DashToHlsStatus DashToHls_ParseLivePssh(struct DashToHlsSession* session,
                                        const uint8_t* bytes, uint64_t length);

// Parses a DASH file from the beginning and generates a single TS segment.
// Live DASH does not use a sidx and includes the the moov atom in each
// segment.  |segment_number| should be unique for each segment and used
// to release the segment.
DashToHlsStatus DashToHls_ParseLive(struct DashToHlsSession* session,
                                    const uint8_t* bytes,
                                    uint64_t length,
                                    uint32_t segment_number,
                                    const uint8_t** hls_segment,
                                    size_t* hls_length);

// Takes one moof/mdat section and converts it to an HLS ts segment.  The
// |hls_segment| is owned by the |session| and freed when ReleaseHlsSegment
// or ReleaseSession is called.
// TODO(justsomeguy) Implement dash_segment_size.  It will be used later to
// allow converting segments as they come in.
DashToHlsStatus DashToHls_ConvertDashSegment(struct DashToHlsSession* session,
                                             uint32_t segment_number,
                                             const uint8_t* dash_segment,
                                             size_t dash_segment_size,
                                             const uint8_t** hls_segment,
                                             size_t* hls_length);

// Takes the actual moof/mdata data in |moof_mdat| and converts it to an HLS ts
// segment. Like DashToHls_ConvertDashSegment, the |segment_number| is owned
// by |session| and is freed in DashToHls_ReleaseHlsSegment.
DashToHlsStatus DashToHls_ConvertDashSegmentData(
    struct DashToHlsSession* session,
    uint32_t segment_number,
    const uint8_t* moof_mdat,
    size_t moof_mdat_size,
    const uint8_t** hls_segment,
    size_t* hls_length);

// Optional call to free up some memory without destroying the entire
// |session|.
DashToHlsStatus DashToHls_ReleaseHlsSegment(struct DashToHlsSession* session,
                                            uint32_t hls_segment_number);

typedef void* DashToHlsContext;
// Common Encryption callbacks.  Common encryption (CENC) at Google is handled
// by a module called the CDM.  Other implementations may use their own DRM
// code to handle the decryption.  This library will call the CENC_PsshHandler
// whenever a pssh box is seen with the entire contents of the box (no header).
//
// Encrypted samples will be passed as just the encrypted portions.  See
// ISO 23001-7 for an explanation of how subsample encryption works.  Or,
// just trust me and AES-128-CTR the entire block passed and don't worry
// about the packing.

typedef DashToHlsStatus (*CENC_PsshHandler)(DashToHlsContext context,
                                            const uint8_t* pssh,
                                            size_t pssh_length);
DashToHlsStatus DashToHls_SetCenc_PsshHandler(struct DashToHlsSession* session,
                                              DashToHlsContext context,
                                              CENC_PsshHandler pssh_handler);

// There are two modes for decrypting.  If useSampleEntries are set then the
// entire data block, clear and encrypted, are sent to the callback with the
// samples set to an array of SampleEntrySize SampleEntrys.
// If use_sample_entries is false then the DashToHls library will concatenate
// all encrypted data, pass just the encrypted data, and reinsert the clear
// content.  If use_sample_entries is false then samples will be nullptr.

struct SampleEntry {
  int32_t clear_bytes;
  int32_t cipher_bytes;
};

typedef DashToHlsStatus (*CENC_DecryptionHandler)(DashToHlsContext context,
                                                  const uint8_t* encrypted,
                                                  uint8_t* clear,
                                                  size_t length,
                                                  uint8_t* iv,
                                                  size_t iv_length,
                                                  const uint8_t* key_id,
                                                  // key_id is always 16 bytes.
                                                  struct SampleEntry*,
                                                  size_t sampleEntrySize);
DashToHlsStatus
DashToHls_SetCenc_DecryptSample(struct DashToHlsSession* session,
                                DashToHlsContext context,
                                CENC_DecryptionHandler decryption_handler,
                                bool use_sample_entries);

// Optional Callback for diagnostic messages.  Returns a structured JSON
// object (C string) with detailed information.
// TODO(justsomeguy) Document json object.
void SetDiagnosticCallback(void (*diagnostic_callback)(const char*));

// Calls the optional callback with a human readable output of the dash
// parsing.
void DashToHls_PrettyPrint(struct DashToHlsSession* session);

#ifdef __cplusplus
};
#endif
#endif  // _UDT_DASH_TRANSMUXER_DASHTOHLSAPI_H_
