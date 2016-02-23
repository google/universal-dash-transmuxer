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

#include "library/ps/program_stream_out.h"

#include "library/ps/nalu.h"
#include "library/ps/pes.h"
#include "library/ps/psm.h"
#include "library/ps/system_header.h"
#include "library/utilities.h"

namespace {
const size_t kPackHeaderLength = 10;
const uint32_t kScrRatio = 300;
const uint32_t kClockRate = 90000;
}  // namespace

namespace dash2hls {

const uint8_t ProgramStreamOut::kPackStartCode[4] = {0x00, 0x00, 0x01, 0xBA};


void ProgramStreamOut::ProcessSample(const uint8_t* input, size_t input_length,
                                     bool is_video,
                                     bool is_sync_sample,
                                     uint64_t pts, uint64_t dts,
                                     uint64_t scr, uint64_t duration,
                                     std::vector<uint8_t>* out) {
  out->resize(0);
  PES pes;
  if (is_video) {
    pes.set_stream_id(PES::kVideoStreamId);
  } else {
    pes.set_stream_id(PES::kAudioStreamId);
  }
  if (is_sync_sample) {
    pes.SetDataAlignmentIndicator(true);
  }
  pes.SetPts(pts);
  if (pts != dts) {
    pes.SetDts(dts);
  }

  std::vector<uint8_t> pes_data(input, input + input_length);
  bool has_aud;
  nalu::PicType pic_type;
  PreprocessNalus(&pes_data, &has_aud, &pic_type);
  AddNeededNalu(&pes_data, pic_type, is_sync_sample, has_aud);
  ConvertLengthToStartCode(&pes_data);
  pes.AddPayload(&pes_data[0], pes_data.size());
  AddHeaders(pes, is_sync_sample, duration, scr, out);
}

void ProgramStreamOut::PreprocessNalus(std::vector<uint8_t>* buffer,
                                       bool* has_aud,
                                       nalu::PicType* pic_type) {
  if (nalu_length_ == 0) {
    return;
  }
  nalu::PreprocessNalus(&(*buffer)[0], buffer->size(), nalu_length_,
                        has_aud, pic_type);
}

void ProgramStreamOut::AddNeededNalu(std::vector<uint8_t>* buffer,
                                     nalu::PicType pic_type,
                                     bool is_sync_sample,
                                     bool has_aud) {
  size_t bytes_to_shift = 0;
  if (!has_aud) {
    bytes_to_shift = nalu::kAudNaluSize + sizeof(uint32_t);
  }

  if (is_sync_sample) {
    bytes_to_shift += sps_pps_.size();
  }

  buffer->resize(buffer->size() + bytes_to_shift);
  memmove(&(*buffer)[bytes_to_shift], &(*buffer)[0],
          buffer->size() - bytes_to_shift);

  // Add the aud nalu to the beginning (see ISO-14496-10).
  if (!has_aud) {
    htonlToBuffer(nalu::kAudNaluSize, &(*buffer)[0]);
    (*buffer)[sizeof(uint32_t)] = nalu::kNaluType_AuDelimiter;
    (*buffer)[sizeof(uint32_t) + 1] = (pic_type << 5) | 0x10;
  }

  if (is_sync_sample) {
    memcpy(&(*buffer)[sizeof(uint32_t) + nalu::kAudNaluSize],
           &sps_pps_[0], sps_pps_.size());
  }
}

void ProgramStreamOut::ConvertLengthToStartCode(std::vector<uint8_t>* buffer) {
  nalu::ReplaceLengthWithStartCode(&(*buffer)[0], buffer->size());
}

void ProgramStreamOut::AddHeaders(const PES& pes,
                                  bool is_sync_sample,
                                  uint64_t duration, uint64_t scr,
                                  std::vector<uint8_t>* out) {
  if (is_sync_sample) {
    SystemHeader system_header;
    PSM psm;
    system_header.AddStream(PES::kPsmStreamId);
    if (has_video_) {
      system_header.AddStream(PES::kVideoStreamId);
      system_header.SetVideoBound(1);
      system_header.SetVideoLockFlag(true);
      psm.AddElementaryStream(PES::kVideoStreamId, PES::kVideoStreamType);
      psm.AddElementaryStreamDescriptor(
          PES::kVideoStreamId, PSM::kVideoAlignmentDescriptor,
          sizeof(PSM::kVideoAlignmentDescriptor));
    }
    if (has_audio_) {
      system_header.AddStream(PES::kAudioStreamId);
      system_header.SetAudioBound(1);
      system_header.SetAudioLockFlag(true);
      psm.AddDescriptor(&audio_oid_[0], audio_oid_.size());
      psm.AddElementaryStream(PES::kAudioStreamId, PES::kAudioStreamType);
      psm.AddElementaryStreamDescriptor(
          PES::kAudioStreamId, PSM::kAudioAlignmentDescriptor,
          sizeof(PSM::kAudioAlignmentDescriptor));
    }
    // These defaults should work in most cases.
    system_header.SetFixedFlag(false);
    system_header.SetCspsFlag(false);
    system_header.SetPacketRestrictionFlag(false);

    psm.set_current_next_indicator(true);
    psm.set_psm_version(0);

    size_t size = GetSizeOfSyncPacket(is_sync_sample, system_header, psm, pes);
    out->resize(size);
    uint8_t *buffer = &(*out)[0];
    uint8_t *end_ptr = &(*out)[out->size()];
    uint32_t mux_rate =
        static_cast<uint32_t>((size * kClockRate) / (50 * duration));
    buffer += WriteHeader(buffer, scr * kScrRatio, mux_rate);
    buffer += system_header.Write(buffer,
                                  static_cast<uint32_t>(end_ptr - buffer));
    buffer += psm.Write(buffer, static_cast<uint32_t>(end_ptr - buffer));
    buffer += pes.Write(buffer, static_cast<uint32_t>(end_ptr - buffer));
    if (buffer != end_ptr) {
      DASH_LOG("ProgramStream AddHeaders failed.",
               "ProgramStream should have added more bytes.",
               "");
    }
  } else {
    size_t size = GetSize(pes);
    out->resize(size);
    uint8_t *buffer = &(*out)[0];
    uint8_t *end_ptr = &(*out)[out->size()];
    uint32_t mux_rate =
        static_cast<uint32_t>((size * kClockRate) / (50 * duration));
    buffer += WriteHeader(buffer, scr * kScrRatio, mux_rate);
    buffer += pes.Write(buffer, static_cast<uint32_t>(end_ptr - buffer));
    if (buffer != end_ptr) {
      DASH_LOG("ProgramStream AddHeaders failed.",
               "ProgramStream should have added more bytes.",
               "");
    }
  }
}

// There are magic numbers in here copied from the Widevine code.
// TODO(justsomeguy) Track down magic numbers and use constants.
size_t ProgramStreamOut::WriteHeader(uint8_t* buffer, uint64_t scr,
                                     uint32_t mux_rate) {
  uint8_t* original_buffer = buffer;
  memcpy(buffer, kPackStartCode, sizeof(kPackStartCode));
  buffer += sizeof(kPackStartCode);
  uint64_t base = scr / kScrRatio;
  uint16_t extension = scr % kScrRatio;
  uint64_t encoded = 0x1;     // "01"
  encoded <<= 3;      // base 32..30
  encoded |= (base >> 30) & 0x07;
  encoded <<= 1;      // marker bit
  encoded |= 0x01;
  encoded <<= 15;     // base 29..15
  encoded |= (base >> 15) & 0x7FFF;
  encoded <<= 1;      // marker bit
  encoded |= 0x01;
  encoded <<= 15;     // base 14..0
  encoded |= base & 0x7FFF;
  encoded <<= 1;      // marker bit
  encoded |= 0x01;
  encoded <<= 9;      // extension 8..0
  encoded |= extension;
  encoded <<= 1;      // marker bit
  encoded |= 0x01;
  encoded <<= 16;     // shift lower 48 bits to the front
  htonllToBuffer(encoded, buffer);
  buffer += 6;
  htonlToBuffer((mux_rate << 10) | 0x00000300, buffer);   // mux rate
  buffer += 3;

  *buffer = 0xF8;  // reserved + stuffing length
  // Insert DVD padding here if we ever want DVD padding.
  ++buffer;
  return buffer - original_buffer;
}

size_t ProgramStreamOut::GetSizeOfSyncPacket(bool is_sync_sample,
                                             const SystemHeader& system_header,
                                             const PSM& psm,
                                             const PES& pes) const {
  size_t result = GetHeaderSize();
  if (is_sync_sample) {
    result += system_header.GetSize() + psm.GetSize();
  }
  result += pes.GetSize();
  return result;
}

size_t ProgramStreamOut::GetSize(const PES& pes) const {
  size_t result = GetHeaderSize();
  result += pes.GetSize();
  return result;
}

size_t ProgramStreamOut::GetHeaderSize() const {
  return sizeof(kPackStartCode) + kPackHeaderLength;
}
}  // namespace dash2hls
