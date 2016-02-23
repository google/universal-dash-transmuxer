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

#include "library/ts/transport_stream_out.h"

#include "library/utilities.h"

namespace {
const uint8_t kAdaptationBit = 0x20;
const uint8_t kAdaptationFiller = 0xff;
const uint8_t kAdaptationPcr = 0x10;
const uint16_t kContinuityMask = 0x0f;
const int64_t kNoPcr = -1;
const uint16_t kPayloadStartBit = 0x4000;
const uint8_t kPayloadBit = 0x10;
const size_t kPcrAdaptationSize = 8;
const size_t kTsPacketSize = 188;
const size_t kTsPayloadSize = 184;
const uint8_t kTsSync = 0x47;
const size_t kAudioFrameHeaderSize = 7;
const size_t kMaxAudioFrameLength = 8191;
}  // namespace

namespace dash2hls {
// The Pat and Pmt for TS are always the same.  There is no need to calculate
// them.
const uint8_t TransportStreamOut::kPat[17] = {
  0x00, 0x00, 0xb0, 0x0d, 0x00, 0x00, 0xc1, 0x00,
  0x00, 0x00, 0x01, 0xe0, 0x20, 0xf9, 0x62, 0xf5,
  0x8b};

const uint8_t TransportStreamOut::kPmtVideo[22] = {
  0x00, 0x02, 0xb0, 0x12, 0x00, 0x01, 0xc1, 0x00,
  0x00, 0xe0, 0x21, 0xf0, 0x00, 0x1b, 0xe0, 0x21,
  0xf0, 0x00, 0x05, 0xcc, 0xcf, 0x0b};

// Audio modifies two bytes at the kPmtAudioConfigOffset meaning a new
// crc has to be calculated.  Everything else stays the same.
// TODO(justsomeguy) See about removing the audio oid.
const uint8_t TransportStreamOut::kPmtAudio[59] = {
  0x00, 0x02, 0xb0, 0x37, 0x00, 0x01, 0xc1, 0x00,
  0x00, 0xff, 0xff, 0xf0, 0x25, 0x1d, 0x23, 0x10,
  0x01, 0x02, 0x1f, 0x00, 0x4f, 0xff, 0xff, 0xfe,
  0xfe, 0xff, 0x03, 0x16, 0x00, 0x22, 0x10, 0x04,
  0x11, 0x40, 0x15, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x05, 0x02,
  0x00, 0x00, 0x0f, 0xe0, 0x22, 0xf0, 0x00, 0x00,
  0x00, 0x00, 0x00};
const size_t kPmtAudioConfigOffset = 48;
const size_t kPmtAudioCrcOffset = 55;

// TODO(justsomeguy) Code copied from the Widevine tree has quite a few
// magic numbers.  This is because it builds up the header by adding bits
// and shifting, adding bits and shifting.  I want to rewrite this to build
// in place and use constants.
void TransportStreamOut::FrameAudio(const uint8_t* input, size_t input_length,
                                    std::vector<uint8_t>* out) const {
  size_t frame_size = input_length + kAudioFrameHeaderSize;
  if (frame_size > kMaxAudioFrameLength) {
    DASH_LOG("Bad audio sample",
             "Audio frames larger than 8191 bytes not supported.",
             DumpMemory(input, input_length).c_str());
    return;
  }
  out->resize(frame_size);
  uint64_t adts_header = 0xfff;
  adts_header <<= 4;
  adts_header |= 0x01;
  adts_header <<= 2;
  adts_header |= (audio_object_type_ - 1);
  adts_header <<= 4;
  adts_header |= sampling_frequency_index_;
  adts_header <<= 4;
  adts_header |= channel_config_;
  adts_header <<= 4;
  adts_header |= 0x0c;
  adts_header <<= 13;
  adts_header |= frame_size;
  adts_header <<= 11;
  adts_header |= 0x7ff;
  adts_header <<=2;
  adts_header <<= 8;
  htonllToBuffer(adts_header, out->data());
  memcpy(out->data() + kAudioFrameHeaderSize, input, input_length);
}

// Modifies the audio_pmt_ to be the kPmtAudio with the correct audio_config.
void TransportStreamOut::set_audio_config(const uint8_t config[2]) {
  // Only way to reset capacity is to swap.
  std::vector<uint8_t>().swap(audio_pmt_);
  audio_pmt_.insert(audio_pmt_.end(), kPmtAudio,
                    kPmtAudio + sizeof(kPmtAudio));
  audio_pmt_[kPmtAudioConfigOffset] = config[0];
  audio_pmt_[kPmtAudioConfigOffset + 1] = config[1];
  htonlToBuffer(static_cast<uint32_t>(wvcrc32(
      &audio_pmt_[1], static_cast<uint32_t>(audio_pmt_.size() - 5))),
                &audio_pmt_[kPmtAudioCrcOffset]);
}

void TransportStreamOut::ProcessSample(const uint8_t* input,
                                       size_t input_length,
                                       bool is_video,
                                       bool is_sync_sample,
                                       uint64_t pts, uint64_t dts,
                                       uint64_t scr, uint64_t duration,
                                       std::vector<uint8_t>* out) {
  // iOS will NOT play with a pts of 0.  So start playing it 1/90,000 of a
  // a second later.
  ++pts;
  ++dts;
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

  std::vector<uint8_t> pes_data;
  if (is_video) {
    pes_data.insert(pes_data.end(), input, input + input_length);
    bool has_aud;
    nalu::PicType pic_type;
    PreprocessNalus(&pes_data, &has_aud, &pic_type);
    AddNeededNalu(&pes_data, pic_type, is_sync_sample, has_aud);
    ConvertLengthToStartCode(&pes_data);
    pes.AddPayload(&pes_data[0], pes_data.size());
  } else {
    FrameAudio(input, input_length, &pes_data);
    pes.AddPayload(&pes_data[0], pes_data.size());
  }

  // out is going to grow a bit, reserve the length now so we don't do any
  // data copies later.  This is longer than it will grow to but not enough
  // larger to care about memory usage.  We may need TS packets for the
  // PAT, PMT, and to finish off the input as well as an extra 4 bytes per
  // packet.  The PES header might push us into one more, so a total of
  // 4 extra TS packets are reserved.  The are small enough to be safe.
  out->resize(0);
  out->reserve(input_length + sizeof(kTsPayloadSize * 4) +
               (input_length / kTsPayloadSize) * 4);
  if (is_sync_sample) {
    OutputRawDataOverTS(kPat, sizeof(kPat), kPidPat, &pat_continuity_counter,
                        out);
    if (is_video) {
      OutputRawDataOverTS(kPmtVideo, sizeof(kPmtVideo), kPidPmt,
                          &pmt_continuity_counter, out);
    } else {
      OutputRawDataOverTS(audio_pmt_.data(), audio_pmt_.size(), kPidPmt,
                          &pmt_continuity_counter, out);
    }
  }
  if (is_video) {
    OutputPesOverTS(pes, kPidVideo, dts, &video_continuity_counter,
                    out);
  } else {
    // TODO(justsomeguy) Make sure kNoPcr doesn't cause jitters.
    OutputPesOverTS(pes, kPidAudio, kNoPcr, &audio_continuity_counter,
                    out);
  }
}

void TransportStreamOut::PreprocessNalus(std::vector<uint8_t>* buffer,
                                         bool* has_aud,
                                         nalu::PicType* pic_type) {
  if (nalu_length_ == 0) {
    return;
  }
  nalu::PreprocessNalus(&(*buffer)[0], buffer->size(), nalu_length_,
                        has_aud, pic_type);
}

void TransportStreamOut::AddNeededNalu(std::vector<uint8_t>* buffer,
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

  // TODO(justsomeguy) Widevine code has it in this order, it should be
  // reversed with the sps_pps before the aud.  It works, but should be
  // fixed to be correct.
  //
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

void TransportStreamOut::ConvertLengthToStartCode(
    std::vector<uint8_t>* buffer) {
  nalu::ReplaceLengthWithStartCode(&(*buffer)[0], buffer->size());
}

// TODO(justsomeguy) Try to copy out common code from the next two routines.
// TODO(justsomeguy) Try to break these routines up for readability.
void TransportStreamOut::OutputRawDataOverTS(const uint8_t* data,
                                             size_t length, uint16_t pid,
                                             uint16_t* continuity_counter,
                                             std::vector<uint8_t>* out) {
  uint32_t num_packets = static_cast<uint32_t>(length / kTsPayloadSize);
  if (length % kTsPayloadSize) {
    ++num_packets;
  }
  size_t original_out_size = out->size();
  out->resize(original_out_size + num_packets * kTsPacketSize);
  uint8_t* write_pointer = &(*out)[original_out_size];
  bool first_packet = true;
  while (num_packets > 0) {
    *write_pointer = kTsSync;
    ++write_pointer;
    if (first_packet) {
      htonsToBuffer(kPayloadStartBit | pid, write_pointer);
      first_packet = false;
    } else {
      htonsToBuffer(pid, write_pointer);
    }
    write_pointer += sizeof(pid);

    size_t adaptation_size = 0;
    size_t bytes_to_write = 0;
    if (length < kTsPayloadSize) {
      adaptation_size = kTsPayloadSize - length;
      bytes_to_write = length;
      length = 0;
      *write_pointer = kAdaptationBit | kPayloadBit;
    } else {
      bytes_to_write = kTsPayloadSize;
      length -= kTsPayloadSize;
      *write_pointer = kPayloadBit;
    }
    *write_pointer |= (*continuity_counter & kContinuityMask);
    ++*continuity_counter;
    ++write_pointer;
    if (adaptation_size > 0) {
      *write_pointer = adaptation_size - 1;
      ++write_pointer;
      --adaptation_size;
      if (adaptation_size > 0) {
        *write_pointer = 0;
        ++write_pointer;
        --adaptation_size;
        memset(write_pointer, kAdaptationFiller, adaptation_size);
        write_pointer += adaptation_size;
      }
    }
    memcpy(write_pointer, data, bytes_to_write);
    data += bytes_to_write;
    write_pointer += bytes_to_write;
    --num_packets;
  }
}

// TODO(justsomeguy)  There's still magic numbers in here.  Really need to
// make them constants.
void TransportStreamOut::OutputPesOverTS(const PES& pes, uint16_t pid,
                                         int64_t pcr,
                                         uint16_t* continuity_counter,
                                         std::vector<uint8_t>* out) {
  size_t length = pes.GetSize();
  if (pcr != kNoPcr) {
    length += kPcrAdaptationSize;
  }
  uint32_t num_packets = static_cast<uint32_t>(length / kTsPayloadSize);
  if (length % kTsPayloadSize) {
    ++num_packets;
  }
  size_t original_out_size = out->size();
  out->resize(original_out_size + num_packets * kTsPacketSize);
  uint8_t* write_pointer = &(*out)[original_out_size];
  bool first_packet = true;
  size_t pes_position = 0;
  while (num_packets > 0) {
    *write_pointer = kTsSync;
    ++write_pointer;
    if (first_packet) {
      htonsToBuffer(kPayloadStartBit | pid, write_pointer);
    } else {
      htonsToBuffer(pid, write_pointer);
    }
    write_pointer += sizeof(pid);

    size_t adaptation_size = 0;
    if (first_packet  && (pcr != kNoPcr)) {
      adaptation_size = kPcrAdaptationSize;
    }
    size_t bytes_to_write = 0;
    if (length < kTsPayloadSize) {
      adaptation_size += kTsPayloadSize - length;
      bytes_to_write = length;
      length = 0;
    } else {
      bytes_to_write = kTsPayloadSize - adaptation_size;
      length -= kTsPayloadSize;
    }
    if (adaptation_size > 0) {
      *write_pointer = kAdaptationBit | kPayloadBit;
    } else {
      *write_pointer = kPayloadBit;
    }
    *write_pointer |= (*continuity_counter & kContinuityMask);
    ++*continuity_counter;
    ++write_pointer;

    if (adaptation_size > 0) {
      *write_pointer = adaptation_size - 1;
      ++write_pointer;
      --adaptation_size;
      if (adaptation_size > 0) {
        if (first_packet && (pcr != kNoPcr)) {
          *write_pointer = kAdaptationPcr;
          ++write_pointer;
          htonlToBuffer(static_cast<uint32_t>(pcr >> 1), write_pointer);
          write_pointer += sizeof(uint32_t);
          *write_pointer = (pcr & 0x01) ? 0x80 : 0x00;
          *write_pointer |= 0x7E;  // 6 reserved bits
          ++write_pointer;
          *write_pointer = 0;
          ++write_pointer;
          adaptation_size -= 7;
        } else {
          *write_pointer = 0;
          ++write_pointer;
          --adaptation_size;
        }
        memset(write_pointer, kAdaptationFiller, adaptation_size);
        write_pointer += adaptation_size;
      }
    }
    pes.WritePartial(write_pointer, pes_position, bytes_to_write);
    pes_position += bytes_to_write;
    write_pointer += bytes_to_write;
    --num_packets;
    first_packet = false;
  }
}
}  // namespace dash2hls
