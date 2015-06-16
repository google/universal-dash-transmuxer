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

#include <map>

#include "include/DashToHlsApi.h"
#include "library/adts/adts_out.h"
#include "library/dash/avcc_contents.h"
#include "library/dash/box.h"
#include "library/dash/box_type.h"
#include "library/dash/dash_parser.h"
#include "library/dash/mdat_contents.h"
#include "library/dash/mp4a_contents.h"
#include "library/dash/mvhd_contents.h"
#include "library/dash/pssh_contents.h"
#include "library/dash/saio_contents.h"
#include "library/dash/saiz_contents.h"
#include "library/dash/sidx_contents.h"
#include "library/dash/tenc_contents.h"
#include "library/dash/tfdt_contents.h"
#include "library/dash/tfhd_contents.h"
#include "library/dash/trun_contents.h"
#include "library/ts/transport_stream_out.h"
#include "utilities.h"

namespace {
const size_t kIvSize = 16;
const size_t kIvCounterOffset = 8;
const size_t kIvCounterSize = 8;
const size_t kDtsClock = 90000;
}  // namespace

namespace dash2hls {
namespace internal {
// ProcessAvcc takes an existing avcC box and fills in the sps_pps.  The
// sps_pps is formated as a blob that can be passed on the TS or PS output.
DashToHlsStatus ProcessAvcc(const AvcCContents* avcc,
                            std::vector<uint8_t>* sps_pps) {
  const std::vector<std::vector<uint8_t> > &
      sequence_set(avcc->get_sequence_parameter_sets());
  const std::vector<std::vector<uint8_t> >&
      picture_set(avcc->get_picture_parameter_sets());

  if ((sequence_set.size() != 1) || (picture_set.size() != 1)) {
    DASH_LOG("Bad avcC box.",
             "avcC box does not have exactly one sequence and one picture.",
             "");
    return kDashToHlsStatus_BadDashContents;
  }

  sps_pps->resize(sequence_set[0].size() + picture_set[0].size() +
                 sizeof(uint32_t) * 2);
  htonlToBuffer(static_cast<uint32_t>(sequence_set[0].size()), &(*sps_pps)[0]);
  memcpy(&(*sps_pps)[sizeof(uint32_t)], &sequence_set[0][0],
         sequence_set[0].size());
  htonlToBuffer(static_cast<uint32_t>(picture_set[0].size()),
                &(*sps_pps)[sequence_set[0].size() + sizeof(uint32_t)]);
  memcpy(&(*sps_pps)[sizeof(uint32_t)*2 + sequence_set[0].size()],
         &picture_set[0][0], picture_set[0].size());
  return kDashToHlsStatus_OK;
}

// Duration of a sample can be either in the individual trun or it can
// use the default set in the tfhd.
uint64_t GetDuration(const TrunContents* trun,
                     const TrunContents::TrackRun* track_run,
                     const TfhdContents* tfhd) {
  uint64_t duration = 0;
  if (trun->IsSampleDurationPresent()) {
    duration = track_run->sample_duration_;
  } else {
    duration = tfhd->get_default_sample_duration();
  }
  if (duration == 0) {
    DASH_LOG("No Duration", "Duration must be greater than 0",
             (trun->BoxName() + ":" + trun->PrettyPrint("") + " " +
              trun->PrettyPrintTrackRun(*track_run)).c_str());
    return kDashToHlsStatus_BadDashContents;
  }
  return duration;
}
}  // namespace internal

// Internal Session object.  Tracks all information used by the calls.
class Session {
 public:
  Session() :
      is_video_(false), is_encrypted_(false), pssh_handler_(nullptr),
      decryption_handler_(nullptr), default_iv_size_(0), nalu_length_(0),
      audio_object_type_(0), sampling_frequency_index_(0), channel_config_(0),
      pssh_context_(nullptr), decryption_context_(nullptr), timescale_(0) {
  }
  bool is_video_;
  DashParser parser_;
  DashToHlsIndex index_;
  bool is_encrypted_;
  CENC_PsshHandler pssh_handler_;
  CENC_DecryptionHandler decryption_handler_;
  size_t default_iv_size_;

  std::map<uint32_t, std::vector<uint8_t> > output_;

  // Video specific settings.
  std::vector<uint8_t> sps_pps_;
  size_t nalu_length_;

  // Audio specific settings.
  uint8_t audio_object_type_;
  uint8_t sampling_frequency_index_;
  uint8_t channel_config_;
  uint8_t audio_config_[2];
  DashToHlsContext pssh_context_;
  DashToHlsContext decryption_context_;
  uint64_t timescale_;
  uint8_t key_id[TencContents::kKidSize];
};

extern "C" DashToHlsStatus
DashToHls_CreateSession(DashToHlsSession** session) {
  Session* dash_session = new Session;
  *session = reinterpret_cast<DashToHlsSession*>(dash_session);
  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
DashToHls_ReleaseSession(DashToHlsSession* session) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  delete dash_session;
  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
DashToHls_ParseDash(DashToHlsSession* session, const uint8_t* bytes,
                    size_t length, DashToHlsIndex** index) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(bytes, length) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }
  const Box* box = dash_session->parser_.Find(BoxType::kBox_sidx);
  if (!box) {
    return kDashToHlsStatus_NeedMoreData;
  }
  const SidxContents* sidx =
      reinterpret_cast<const SidxContents*>(box->get_contents());
  if (!sidx) {
    return kDashToHlsStatus_BadDashContents;
  }
  const std::vector<DashToHlsSegment>& locations(sidx->get_locations());
  dash_session->index_.index_count = static_cast<uint32_t>(locations.size());
  dash_session->index_.segments = &locations[0];
  *index = &dash_session->index_;

  const MvhdContents* mvhd = nullptr;
  box = dash_session->parser_.FindDeep(BoxType::kBox_mvhd);
  if (!box) {
    DASH_LOG("Bad Dash Content.", "No mvhd", "");
    return kDashToHlsStatus_BadDashContents;
  }
  mvhd = reinterpret_cast<const MvhdContents*>(box->get_contents());
  dash_session->timescale_ = mvhd->get_timescale();
  if (dash_session->timescale_ == 0) {
    DASH_LOG("Bad Dash Content.", "mvhd needs a timescale.", "");
    return kDashToHlsStatus_BadDashContents;
  }

  // See if we have an video box.
  box = dash_session->parser_.FindDeep(BoxType::kBox_avcC);
  if (!box) {
    box = dash_session->parser_.FindDeep(BoxType::kBox_encv);
  }
  if (box) {
    dash_session->is_video_ = true;
    const AvcCContents *avcc =
        reinterpret_cast<const AvcCContents*>(box->get_contents());
    if (internal::ProcessAvcc(avcc, &dash_session->sps_pps_)
        != kDashToHlsStatus_OK) {
      return kDashToHlsStatus_BadDashContents;
    }
    dash_session->nalu_length_ = avcc->GetNaluLength();
  } else {
    // No video box, find the audio box.
    // In theory we could have both audio and video, but for now we only
    // support one or the other.
    box = dash_session->parser_.FindDeep(BoxType::kBox_mp4a);
    if (!box) {
      box = dash_session->parser_.FindDeep(BoxType::kBox_enca);
      if (!box) {
        return kDashToHlsStatus_BadDashContents;
      }
    }
    dash_session->is_video_ = false;
    const Mp4aContents *mp4a =
        reinterpret_cast<const Mp4aContents*>(box->get_contents());
    dash_session->audio_object_type_ = mp4a->get_audio_object_type();
    dash_session->sampling_frequency_index_ =
        mp4a->get_sampling_frequency_index();
    dash_session->channel_config_ = mp4a->get_channel_config();
    dash_session->audio_config_[0] = mp4a->get_audio_config()[0];
    dash_session->audio_config_[1] = mp4a->get_audio_config()[1];
  }

  // Check for CENC.
  box = dash_session->parser_.FindDeep(BoxType::kBox_tenc);
  if (!box) {
    return kDashToHlsStatus_ClearContent;
  }
  const TencContents* tenc =
      reinterpret_cast<const TencContents*>(box->get_contents());

  box = dash_session->parser_.FindDeep(BoxType::kBox_pssh);
  if (!box) {
    DASH_LOG("Missing boxes.", "Missing pssh box", "");
    return kDashToHlsStatus_BadConfiguration;
  }
  const PsshContents* pssh =
      reinterpret_cast<const PsshContents*>(box->get_contents());

  if (!dash_session->pssh_handler_ || !dash_session->decryption_handler_) {
    DASH_LOG("Bad Configuration.", "Missing required callback for CENC",
             "");
    return kDashToHlsStatus_BadConfiguration;
  }
  dash_session->is_encrypted_ = true;
  std::vector<uint8_t> pssh_box;
  dash_session->pssh_handler_(dash_session->pssh_context_,
                              pssh->get_full_box().data(),
                              pssh->get_full_box().size());
  // TODO(justsomeguy) support more lengths than 8.
  if (tenc->get_default_iv_size() != 8) {
    DASH_LOG("Unimplemented.",
             "Currently only implements a default IV of 8.",
             "");
  }
  dash_session->default_iv_size_ = tenc->get_default_iv_size();
  memcpy(dash_session->key_id, tenc->get_default_kid(),
         TencContents::kKidSize);


  return kDashToHlsStatus_OK;
}

namespace {
// For any section we need to get all of the boxes in this routine.  If any
// are missing then it's bad content.
DashToHlsStatus GetNeededBoxes(bool is_encrypted,
                               size_t index,
                               const DashParser& parser,
                               const MdatContents** mdat,
                               const BoxContents** moof,
                               const TfdtContents** tfdt,
                               const TfhdContents** tfhd,
                               const TrunContents** trun,
                               const SaioContents** saio,
                               const SaizContents** saiz,
                               const TencContents** tenc) {
  if (mdat) {
    std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_mdat);
    if (boxes.size() <= index) {
      if (index > 0) {
        return kDashToHlsStatus_NeedMoreData;
      }
      DASH_LOG("Bad Dash Content.", "No mdat", parser.PrettyPrint("").c_str());
      return kDashToHlsStatus_BadDashContents;
    }
    *mdat = reinterpret_cast<const MdatContents*>(boxes[index]->get_contents());
    if (*mdat == nullptr) {
      return kDashToHlsStatus_NeedMoreData;
    }
  }

  if (moof) {
    std::vector<const Box*> boxes = parser.FindAll(BoxType::kBox_moof);
    if (boxes.size() <= index) {
      DASH_LOG("Bad Dash Content.", "No moof", "");
      return kDashToHlsStatus_BadDashContents;
    }
    *moof = reinterpret_cast<const BoxContents*>(boxes[index]->get_contents());
  }

  if (tfdt) {
    std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_tfdt);
    if (boxes.size() <= index) {
      DASH_LOG("Bad Dash Content.", "No tfdt", "");
      return kDashToHlsStatus_BadDashContents;
    }
    *tfdt = reinterpret_cast<const TfdtContents*>(boxes[index]->get_contents());
  }

  if (tfhd) {
    std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_tfhd);
    if (boxes.size() <= index) {
      DASH_LOG("Bad Dash Content.", "No tfhd", "");
      return kDashToHlsStatus_BadDashContents;
    }
    *tfhd = reinterpret_cast<const TfhdContents*>(boxes[index]->get_contents());
  }

  if (trun) {
    std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_trun);
    if (boxes.size() <= index) {
      DASH_LOG("Bad Dash Content.", "No trun", "");
      return kDashToHlsStatus_BadDashContents;
    }
    *trun = reinterpret_cast<const TrunContents*>(boxes[index]->get_contents());
  }

  if (is_encrypted) {
    // These boxes are allowed to be missing.
    if (saio) {
      std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_saio);
      if (boxes.size() > index) {
        *saio = reinterpret_cast<const SaioContents*>(
            boxes[index]->get_contents());
      } else {
        *saio = nullptr;
      }
    }

    if (saiz) {
      std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_saiz);
      if (boxes.size() > index) {
        *saiz = reinterpret_cast<const SaizContents*>(
            boxes[index]->get_contents());
      } else {
        *saiz = nullptr;
      }
    }

    if (tenc) {
      std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_tenc);
      if (boxes.size() > index) {
        *tenc = reinterpret_cast<const TencContents*>(
            boxes[index]->get_contents());
      } else {
        *tenc = nullptr;
      }
    }
  }

  return kDashToHlsStatus_OK;
}

// TODO(justsomeguy) The audio samples have a size of 8 and that's making
// this routine ugly.  Need to clean it up and make it pretty.
//
// TODO(justsomeguy) Clean up this routine so it's easy to understand.  Right
// now it looks like magic.
bool DecryptSample(const Session* session, uint32_t sample_number,
                   const SaizContents* saiz, const SaioContents* saio,
                   const uint8_t* key_id,
                   const MdatContents* mdat, uint64_t mdat_offset,
                   uint32_t sample_size, uint64_t* saio_position,
                   std::vector<uint8_t>* out) {
  if (saiz->get_sizes().size() <= sample_number) {
    DASH_LOG("Unsupported saiz.",
             "Only supports CENC for ALL samples.",
             "");
    return false;
  }
  if (session->default_iv_size_ != kIvSize - kIvCounterOffset) {
    DASH_LOG("Bad IV.",
             "Unexpected default_iv_size_ size.",
             "");
    return false;
  }
  uint8_t iv[kIvSize];
  size_t size = saiz->get_sizes()[sample_number];
  if ((size != 8) &&
      ((size - 8 - sizeof(uint16_t)) % SaizContents::SaizRecordSize)) {
    DASH_LOG("Bad saiz.",
             "saiz box must be a multiple of SaizRecord sizes.",
             "");
    return false;
  }
  const uint8_t* mdat_end = mdat->get_raw_data() + mdat->get_raw_data_length();
  const uint8_t* mdat_data = mdat->get_raw_data();
  if (mdat_data + *saio_position + session->default_iv_size_ > mdat_end) {
    DASH_LOG("Bad saio.",
             "saio position would run off the end.",
             "");
    return false;
  }
  memcpy(iv, mdat_data + *saio_position, session->default_iv_size_);
  memset(iv + kIvCounterOffset, 0, kIvCounterSize);
  *saio_position += session->default_iv_size_;
  size_t saio_records = 1;
  if (size > 8) {
    if (mdat_data + *saio_position + sizeof(uint16_t) > mdat_end) {
      DASH_LOG("Bad saio.",
               "saio position would run off the end.",
               "");
      return false;
    }
    saio_records = ntohsFromBuffer(mdat_data + *saio_position);
    *saio_position += sizeof(uint16_t);
  }

  const SaizContents::SaizRecord* record = reinterpret_cast<
      const SaizContents::SaizRecord*>(mdat_data + *saio_position);
  if (mdat_data + *saio_position +
      (sizeof(SaizContents::SaizRecord) * saio_records) > mdat_end) {
    DASH_LOG("Bad saio.",
             "saio records are not in mdat.",
             "");
    return false;
  }
  size_t encrypted_position = 0;
  size_t mdat_position = mdat_offset;
  std::vector<uint8_t> encrypted_buffer;
  encrypted_buffer.resize(sample_size);
  // TODO(justsomeguy) don't walk off the end.
  for (size_t count = 0; count < saio_records; ++count) {
    size_t clear_bytes = 0;
    size_t encrypted_bytes = sample_size;
    if (size > 8) {
      clear_bytes = record[count].clear_bytes();
      encrypted_bytes = record[count].encrypted_bytes();
    }
    mdat_position += clear_bytes;
    if (mdat_position + encrypted_bytes > mdat->get_raw_data_length()) {
      std::string error_msg =
          "mdat_position(" + std::to_string(mdat_position) +
          ") + encrypted_bytes(" + std::to_string(encrypted_bytes) + ") > " +
          "mdat->get_raw_data_length(" +
          std::to_string(mdat->get_raw_data_length()) + ")";
      DASH_LOG("Buffer overrun.", error_msg.c_str(), "");
      return false;
    }
    if (encrypted_position + encrypted_bytes > sample_size) {
      DASH_LOG("Bad saio.",
               "Encrypted bytes cause a buffer overlow.",
               "");
      return false;
    }
    memcpy(&encrypted_buffer[encrypted_position], mdat_data + mdat_position,
           encrypted_bytes);
    mdat_position += encrypted_bytes;
    encrypted_position += encrypted_bytes;
    if (size >8) {
      *saio_position += SaizContents::SaizRecordSize;
    }
  }
  std::vector<uint8_t> clear_buffer;
  clear_buffer.resize(encrypted_position);
  if (session->decryption_handler_(session->decryption_context_,
                                   &encrypted_buffer[0], &clear_buffer[0],
                                   encrypted_position, iv, sizeof(iv),
                                   key_id, nullptr, 0) !=
      kDashToHlsStatus_OK) {
    return false;
  }
  out->resize(sample_size);

  // Putting things back should not be able to cause a buffer overflow.

  size_t decrypted_position = 0;
  encrypted_position = 0;
  mdat_position = mdat_offset;
  for (size_t count = 0; count < saio_records; ++count) {
    size_t clear_bytes = 0;
    size_t encrypted_bytes = sample_size;
    if (size > 8) {
      clear_bytes = record[count].clear_bytes();
      encrypted_bytes = record[count].encrypted_bytes();
    }
    if (mdat_position + clear_bytes > mdat->get_raw_data_length()) {
      std::string error_msg =
          "mdat_position(" + std::to_string(mdat_position) +
          ") + clear_bytes(" + std::to_string(clear_bytes) + ") > " +
          "mdat->get_raw_data_length(" +
          std::to_string(mdat->get_raw_data_length()) + ")";
      DASH_LOG("Buffer overrun.", error_msg.c_str(), "");
      return false;
    }
    memcpy(&(*out)[decrypted_position], mdat_data + mdat_position,
           clear_bytes);
    mdat_position += clear_bytes;
    decrypted_position += clear_bytes;
    memcpy(&(*out)[decrypted_position], &clear_buffer[encrypted_position],
           encrypted_bytes);
    mdat_position += encrypted_bytes;
    encrypted_position += encrypted_bytes;
    decrypted_position += encrypted_bytes;
  }
  return true;
}

DashToHlsStatus TransmuxToTS(const Session* dash_session,
                             const MdatContents* mdat,
                             const BoxContents* moof,
                             const TfdtContents* tfdt,
                             const TfhdContents* tfhd,
                             const TrunContents* trun,
                             const SaioContents* saio,
                             const SaizContents* saiz,
                             const TencContents* tenc,
                             std::vector<uint8_t>* ts_output) {
  ts_output->erase(ts_output->begin(), ts_output->end());

  TransportStreamOut ts_out;
  AdtsOut adts_out;
  if (dash_session->is_video_) {
    ts_out.set_sps_pps(dash_session->sps_pps_);
    ts_out.set_nalu_length(dash_session->nalu_length_);
  } else {
    adts_out.set_audio_object_type(dash_session->audio_object_type_);
    adts_out.set_sampling_frequency_index(
        dash_session->sampling_frequency_index_);
    adts_out.set_channel_config(dash_session->channel_config_);
  }

  uint64_t saio_position = 0;
  // Not all segments are encrypted, there can be a clear lead.
  if (saio && saiz) {
    if (saio->get_offsets().size() != 1) {
      DASH_LOG("Bad Saio.",
               "Only supports contiguous offsets.",
               "");
      return kDashToHlsStatus_BadDashContents;
    }
    saio_position = moof->get_stream_position() + saio->get_offsets()[0] -
      sizeof(uint32_t) * 2 - mdat->get_stream_position();
  }
  const std::vector<TrunContents::TrackRun>& track_run =
      trun->get_track_runs();

  uint64_t dts = (tfdt->get_base_media_decode_time() * kDtsClock) /
      dash_session->timescale_;
  std::vector<uint8_t> output;
  if (!dash_session->is_video_) {
    adts_out.AddTimestamp(dts, ts_output);
  }
  // Complicated way to get the value that's almost always going to be 0.
  // The definition of the start of the samples is the data offset in the
  // trun plus the start of the moof after the header (sizeof(uint32_t)*2).
  // TODO(justsomeguy) The tfhd can set the base-data-offset to something
  // besides the start of the moof.
  uint64_t mdat_offset =
      moof->get_stream_position() + trun->get_data_offset() -
      sizeof(uint32_t) * 2 -
      mdat->get_stream_position();
  const uint8_t* mdat_data = mdat->get_raw_data();
  size_t sample_number = 0;
  for (std::vector<TrunContents::TrackRun>::const_iterator
           iter = track_run.begin(); iter != track_run.end(); ++iter) {
    uint64_t duration =
        (internal::GetDuration(trun, &(*iter), tfhd) * kDtsClock) /
        dash_session->timescale_;
    if (duration == 0) {
      return kDashToHlsStatus_BadDashContents;
    }
    uint64_t pts = dts;
    if (trun->IsSampleCompositionPresent()) {
      pts += (iter->sample_composition_time_offset_ * kDtsClock)
          / dash_session->timescale_;
    }
    if (mdat_offset + iter->sample_size_ > mdat->get_raw_data_length()) {
      DASH_LOG("Buffer overrun.", "Offset would be past the end of the mdat.",
               "");
      return kDashToHlsStatus_BadDashContents;
    }
    std::vector<uint8_t> decrypted;
    if (saio && saiz) {
      const uint8_t* key_id = nullptr;
      if (tenc) {
        key_id = tenc->get_default_kid();
      } else {
        key_id = dash_session->key_id;
      }

      if (!DecryptSample(dash_session, sample_number, saiz, saio, key_id, mdat,
                         mdat_offset, iter->sample_size_, &saio_position,
                         &decrypted)) {
        return kDashToHlsStatus_BadDashContents;
      }
      if (dash_session->is_video_) {
      ts_out.ProcessSample(decrypted.data(), decrypted.size(),
                           dash_session->is_video_, sample_number == 0,
                           pts, dts, dts, duration, &output);
    } else {
        adts_out.ProcessSample(decrypted.data(), decrypted.size(), &output);
      }
    } else {
      if (dash_session->is_video_) {
      ts_out.ProcessSample(mdat_data + mdat_offset, iter->sample_size_,
                           dash_session->is_video_, sample_number == 0,
                           pts, dts, dts, duration, &output);
      } else {
        adts_out.ProcessSample(mdat_data + mdat_offset, iter->sample_size_,
                               &output);
    }
    }
    ++sample_number;
    ts_output->insert(ts_output->end(), output.begin(), output.end());
    mdat_offset += iter->sample_size_;
    dts += duration;
  }
  return kDashToHlsStatus_OK;
}
}  // namespace

extern "C" DashToHlsStatus
DashToHls_ParseSidx(DashToHlsSession* session, const uint8_t* bytes,
                    uint64_t length, DashToHlsIndex** index) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(bytes, length) == 0) {
    DASH_LOG("Bad Dash Content.", "Unable to parse for sidx", "");
    return kDashToHlsStatus_BadDashContents;
  }
  const Box* box = dash_session->parser_.Find(BoxType::kBox_sidx);
  if (!box) {
    DASH_LOG("Bad Dash Content.", "Missing sidx box", "");
    return kDashToHlsStatus_BadDashContents;
  }
  const SidxContents* sidx =
      reinterpret_cast<const SidxContents*>(box->get_contents());
  if (!sidx) {
    DASH_LOG("Bad Dash Content.", "Could not retrieve SidxContents", "");
    return kDashToHlsStatus_BadDashContents;
  }
  const std::vector<DashToHlsSegment>& locations(sidx->get_locations());
  dash_session->index_.index_count = static_cast<uint32_t>(locations.size());
  dash_session->index_.segments = &locations[0];
  *index = &dash_session->index_;

  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
DashToHls_ParseLivePssh(DashToHlsSession* session, const uint8_t* bytes,
                        uint64_t length) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(bytes, length) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }

  // Check for CENC.
  const Box* box = dash_session->parser_.FindDeep(BoxType::kBox_tenc);
  if (box) {
    box = dash_session->parser_.FindDeep(BoxType::kBox_pssh);
    if (!box) {
      DASH_LOG("Missing boxes.", "Missing pssh box", "");
      return kDashToHlsStatus_BadConfiguration;
    }
    const PsshContents* pssh =
        reinterpret_cast<const PsshContents*>(box->get_contents());

    if (!dash_session->pssh_handler_ || !dash_session->decryption_handler_) {
      DASH_LOG("Bad Configuration.", "Missing required callback for CENC",
               "");
      return kDashToHlsStatus_BadConfiguration;
    }
    dash_session->is_encrypted_ = true;
    dash_session->pssh_handler_(dash_session->pssh_context_,
                                pssh->get_full_box().data(),
                                pssh->get_full_box().size());
  } else {
    return kDashToHlsStatus_ClearContent;
  }
  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
DashToHls_ParseLive(DashToHlsSession* session, const uint8_t* bytes,
                    uint64_t length,
                    uint64_t segment_number,
                    const uint8_t** hls_segment,
                    size_t* hls_length) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(bytes, length) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }

  // Check for CENC.
  const Box* box = dash_session->parser_.FindDeep(BoxType::kBox_tenc);
  if (box) {
    const TencContents* tenc =
        reinterpret_cast<const TencContents*>(box->get_contents());

    box = dash_session->parser_.FindDeep(BoxType::kBox_pssh);
    if (!box) {
      DASH_LOG("Missing boxes.", "Missing pssh box", "");
      return kDashToHlsStatus_BadConfiguration;
    }

    if (!dash_session->pssh_handler_ || !dash_session->decryption_handler_) {
      DASH_LOG("Bad Configuration.", "Missing required callback for CENC",
               "");
      return kDashToHlsStatus_BadConfiguration;
    }
    dash_session->is_encrypted_ = true;
    // TODO(justsomeguy) support more lengths than 8.
    if (tenc->get_default_iv_size() != 8) {
      DASH_LOG("Unimplemented.",
               "Currently only implements a default IV of 8.",
               "");
    }
    dash_session->default_iv_size_ = tenc->get_default_iv_size();
  }

  // See if we have an video box.
  box = dash_session->parser_.FindDeep(BoxType::kBox_avcC);
  if (!box) {
    box = dash_session->parser_.FindDeep(BoxType::kBox_encv);
  }
  if (box) {
    dash_session->is_video_ = true;
    const AvcCContents *avcc =
        reinterpret_cast<const AvcCContents*>(box->get_contents());
    if (internal::ProcessAvcc(avcc, &dash_session->sps_pps_)
        != kDashToHlsStatus_OK) {
      return kDashToHlsStatus_BadDashContents;
    }
    dash_session->nalu_length_ = avcc->GetNaluLength();
  } else {
    // No video box, find the audio box.
    // In theory we could have both audio and video, but for now we only
    // support one or the other.
    box = dash_session->parser_.FindDeep(BoxType::kBox_mp4a);
    if (!box) {
      box = dash_session->parser_.FindDeep(BoxType::kBox_enca);
      if (!box) {
        return kDashToHlsStatus_BadDashContents;
      }
    }
    dash_session->is_video_ = false;
    const Mp4aContents *mp4a =
        reinterpret_cast<const Mp4aContents*>(box->get_contents());
    dash_session->audio_object_type_ = mp4a->get_audio_object_type();
    dash_session->sampling_frequency_index_ =
        mp4a->get_sampling_frequency_index();
    dash_session->channel_config_ = mp4a->get_channel_config();
    dash_session->audio_config_[0] = mp4a->get_audio_config()[0];
    dash_session->audio_config_[1] = mp4a->get_audio_config()[1];
  }

  const MdatContents* mdat = nullptr;
  const BoxContents* moof = nullptr;
  const TfdtContents* tfdt = nullptr;
  const TfhdContents* tfhd = nullptr;
  const TrunContents* trun = nullptr;
  const SaioContents* saio = nullptr;
  const SaizContents* saiz = nullptr;
  const TencContents* tenc = nullptr;
  DashToHlsStatus result = GetNeededBoxes(dash_session->is_encrypted_,
                                          0,
                                          dash_session->parser_,
                                          &mdat, &moof, &tfdt, &tfhd,
                                          &trun, &saio, &saiz, &tenc);
  if (result != kDashToHlsStatus_OK) {
    return result;
  }

  const MvhdContents* mvhd = nullptr;
  box = dash_session->parser_.FindDeep(BoxType::kBox_mvhd);
  if (!box) {
    DASH_LOG("Bad Dash Content.", "No mvhd", "");
    return kDashToHlsStatus_BadDashContents;
  }
  mvhd = reinterpret_cast<const MvhdContents*>(box->get_contents());
  dash_session->timescale_ = mvhd->get_timescale();
  if (dash_session->timescale_ == 0) {
    DASH_LOG("Bad Dash Content.", "mvhd needs a timescale.", "");
    return kDashToHlsStatus_BadDashContents;
  }

  result = TransmuxToTS(dash_session, mdat, moof, tfdt,
                        tfhd, trun, saio, saiz, tenc,
                        &dash_session->output_[segment_number]);
  if (result == kDashToHlsStatus_OK) {
    *hls_segment = &dash_session->output_[segment_number][0];
    *hls_length = dash_session->output_[segment_number].size();
  }
  return result;
}

extern "C" DashToHlsStatus
DashToHls_ConvertDashSegmentData(DashToHlsSession* session,
                                 uint32_t segment_number,
                                 const uint8_t* moof_mdat,
                                 size_t moof_mdat_size,
                                 const uint8_t** hls_segment,
                                 size_t* hls_length) {
  const MdatContents* mdat = nullptr;
  const BoxContents* moof = nullptr;
  const TfdtContents* tfdt = nullptr;
  const TfhdContents* tfhd = nullptr;
  const TrunContents* trun = nullptr;
  const SaioContents* saio = nullptr;
  const SaizContents* saiz = nullptr;
  const TencContents* tenc = nullptr;

  Session* dash_session = reinterpret_cast<Session*>(session);
  const Box* box = dash_session->parser_.FindDeep(BoxType::kBox_tenc);
  if (box) {
    tenc = reinterpret_cast<const TencContents*>(box->get_contents());
  }

  DashParser moof_mdat_parser;
  if (moof_mdat_parser.Parse(moof_mdat, moof_mdat_size) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }
  DashToHlsStatus result = GetNeededBoxes(dash_session->is_encrypted_,
                                          0, moof_mdat_parser,
                                          &mdat, &moof, &tfdt, &tfhd,
                                          &trun, &saio, &saiz, nullptr);
  if (result != kDashToHlsStatus_OK) {
    return result;
  }

  result = TransmuxToTS(dash_session, mdat,  moof, tfdt, tfhd,
                        trun, saio, saiz, tenc,
                        &dash_session->output_[segment_number]);
  if (result == kDashToHlsStatus_OK) {
    *hls_segment = &dash_session->output_[segment_number][0];
    *hls_length = dash_session->output_[segment_number].size();
  }
  return result;
}

extern "C" DashToHlsStatus
DashToHls_ConvertDashSegment(DashToHlsSession* session,
                             uint32_t segment_number,
                             const uint8_t* dash_segment,
                             size_t dash_segment_size,
                             const uint8_t** hls_segment,
                             size_t* hls_length) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  DashParser parser;
  // The parser relies on offsets from the beginning of the file.
  parser.set_current_position(
      dash_session->index_.segments[segment_number].location);
  if (parser.Parse(dash_segment,
          dash_session->index_.segments[segment_number].length) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }

  const MdatContents* mdat = nullptr;
  const BoxContents* moof = nullptr;
  const TfdtContents* tfdt = nullptr;
  const TfhdContents* tfhd = nullptr;
  const TrunContents* trun = nullptr;
  const SaioContents* saio = nullptr;
  const SaizContents* saiz = nullptr;
  const TencContents* tenc = nullptr;
  size_t segment_count = 0;

  while (true) {
    DashToHlsStatus result = GetNeededBoxes(dash_session->is_encrypted_,
                                            segment_count, parser,
                                            &mdat, &moof, &tfdt, &tfhd,
                                            &trun, &saio, &saiz, &tenc);
    if (result != kDashToHlsStatus_OK) {
      if (result == kDashToHlsStatus_NeedMoreData) {
        *hls_segment = &dash_session->output_[segment_number][0];
        *hls_length = dash_session->output_[segment_number].size();
        return kDashToHlsStatus_OK;
      }
      return result;
    }
    ++segment_count;

    result = TransmuxToTS(dash_session, mdat,  moof, tfdt, tfhd,
                          trun, saio, saiz, tenc,
                          &dash_session->output_[segment_number]);
    if (result != kDashToHlsStatus_OK) {
      return result;
    }
  }
  return kDashToHlsStatus_OK;
}

extern "C"
DashToHlsStatus DashToHls_ReleaseHlsSegment(DashToHlsSession* session,
                                            uint32_t hls_segment_number) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  dash_session->output_[hls_segment_number] = std::vector<uint8_t>();
  return kDashToHlsStatus_OK;
}

extern "C"
void SetDiagnosticCallback(void (*diagnostic_callback)(const char*)) {
  if (diagnostic_callback) {
    g_diagnostic_callback = diagnostic_callback;
  } else {
    g_diagnostic_callback = DashToHlsDefaultDiagnosticCallback;
  }
}

extern "C"
void DashToHls_PrettyPrint(DashToHlsSession* session) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  g_diagnostic_callback(dash_session->parser_.PrettyPrint("").c_str());
}

extern "C"
DashToHlsStatus DashToHls_SetCenc_PsshHandler(DashToHlsSession* session,
                                              DashToHlsContext context,
                                              CENC_PsshHandler pssh_handler) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  dash_session->pssh_handler_ = pssh_handler;
  dash_session->pssh_context_ = context;
  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
DashToHls_SetCenc_DecryptSample(DashToHlsSession* session,
                                DashToHlsContext context,
                                CENC_DecryptionHandler decryption_handler,
                                bool use_sample_entries) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  dash_session->decryption_handler_ = decryption_handler;
  dash_session->decryption_context_ = context;
  return kDashToHlsStatus_OK;
}
}  // namespace dash2hls
