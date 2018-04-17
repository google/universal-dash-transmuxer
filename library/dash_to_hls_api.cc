/*
Copyright 2018 Google Inc. All rights reserved.

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

#ifdef USE_AVFRAMEWORK
#include "library/dash_to_hls_api_avframework.h"
#endif

#include "include/DashToHlsApi.h"
#include "library/adts/adts_out.h"
#include "library/dash/avcc_contents.h"
#include "library/dash/box.h"
#include "library/dash/box_type.h"
#include "library/dash/dash_parser.h"
#include "library/dash/mdat_contents.h"
#include "library/dash/mdhd_contents.h"
#include "library/dash/mp4a_contents.h"
#include "library/dash/mvhd_contents.h"
#include "library/dash/pssh_contents.h"
#include "library/dash/saio_contents.h"
#include "library/dash/saiz_contents.h"
#include "library/dash/sbgp_contents.h"
#include "library/dash/sgpd_contents.h"
#include "library/dash/sidx_contents.h"
#include "library/dash/tfdt_contents.h"
#include "library/dash/tfhd_contents.h"
#include "library/dash/trex_contents.h"
#include "library/dash/trun_contents.h"
#include "library/dash_to_hls_session.h"
#include "library/ts/transport_stream_out.h"
#include "library/utilities.h"

namespace {
const size_t kIvSize = 16;
const size_t kDtsClock = 90000;
const uint32_t kSeigGroupType = 0x73656967;
const uint32_t kGroupIndex = 0x10001;

}  // namespace

namespace dash2hls {
namespace internal {
// ProcessAvcc takes an existing avcC box and fills in the sps_pps.  The
// sps_pps is formated as a blob that can be passed on the TS or PS output.
DashToHlsStatus ProcessAvcc(const AvcCContents* avcc,
                            uint8_t stream_number,
                            std::vector<uint8_t>* sps_pps) {
  const std::vector<std::vector<uint8_t> > &
      sequence_set(avcc->get_sequence_parameter_sets());
  const std::vector<std::vector<uint8_t> >&
      picture_set(avcc->get_picture_parameter_sets());

  // Override Stream Number.
  // Enables INIT segments that are used for a single stream.
  if (picture_set.size() == 1) {
    stream_number = 0;
  }
  sps_pps->resize(sequence_set[stream_number].size() +
                  picture_set[stream_number].size() +
                 sizeof(uint32_t) * 2);
  htonlToBuffer(static_cast<uint32_t>(sequence_set[stream_number].size()),
                &(*sps_pps)[0]);
  memcpy(&(*sps_pps)[sizeof(uint32_t)], &sequence_set[stream_number][0],
         sequence_set[stream_number].size());
  htonlToBuffer(static_cast<uint32_t>(picture_set[stream_number].size()),
                &(*sps_pps)[sequence_set[stream_number].size() +
                            sizeof(uint32_t)]);
  memcpy(&(*sps_pps)[sizeof(uint32_t) * 2 + sequence_set[stream_number].size()],
         &picture_set[stream_number][0], picture_set[stream_number].size());
  return kDashToHlsStatus_OK;
}

// Duration of a sample can be either in the individual trun or it can
// use the default set in the tfhd.
uint64_t GetDuration(const TrunContents* trun,
                     const TrunContents::TrackRun* track_run,
                     const TfhdContents* tfhd,
                     uint64_t trex_default_sample_duration) {
  uint64_t duration = 0;
  if (trun->IsSampleDurationPresent()) {
    duration = track_run->sample_duration_;
  } else if (tfhd->IsDefaultSampleDurationPresent()) {
    duration = tfhd->get_default_sample_duration();
  } else {
    duration = trex_default_sample_duration;
  }
  if (duration == 0) {
    if (track_run) {
      DASH_LOG("No Duration", "Duration must be greater than 0",
               (trun->BoxName() + ":" + trun->PrettyPrint("") + " " +
                trun->PrettyPrintTrackRun(*track_run) + " " +
                PrettyPrintValue(trex_default_sample_duration)).c_str());
    } else {
      DASH_LOG("No Duration", "Duration must be greater than 0",
               (trun->BoxName() + ":" + trun->PrettyPrint("") + " " +
                PrettyPrintValue(trex_default_sample_duration)).c_str());
    }
    return kDashToHlsStatus_BadDashContents;
  }
  return duration;
}

bool UpdateSessionKeyId(Session* session,
                        const DashParser& parser) {

  std::vector<const Box*> sgpd_boxes = parser.FindDeepAll(BoxType::kBox_sgpd);
  std::vector<const Box*> sbgp_boxes = parser.FindDeepAll(BoxType::kBox_sbgp);

  const SbgpContents* seig_sbgp = nullptr;
  const SgpdContents* seig_sgpd = nullptr;
  for (auto iter = sbgp_boxes.begin(); iter != sbgp_boxes.end(); ++iter) {
    const SbgpContents* sbgp =
        reinterpret_cast<const SbgpContents*>((*iter)->get_contents());
    if (sbgp->get_grouping_type() != kSeigGroupType) continue;
    seig_sbgp = sbgp;
    break;
  }
  for (auto iter = sgpd_boxes.begin(); iter != sgpd_boxes.end(); ++iter) {
    const SgpdContents* sgpd =
        reinterpret_cast<const SgpdContents*>((*iter)->get_contents());
    if (sgpd->get_grouping_type() != kSeigGroupType) continue;
    seig_sgpd = sgpd;
    break;
  }
  // Check for SBGP and SGPD were found and parsed.
  if (!seig_sbgp && !seig_sgpd) {
    return true;
  }
  // Both SBGP and SGPD must be present if one is.
  if (!seig_sbgp || !seig_sgpd) {
    DASH_LOG("Unsupported DASH Config", "Missing SBGP or SGPD Box", "");
    return false;
  }
  // Only group index 0x10001 is supported.
  if (seig_sbgp->get_description_index() != kGroupIndex) {
    DASH_LOG("Unsupported DASH Config", "Wrong SEIG Index", "");
    return false;
  }
  memcpy(session->key_id_, seig_sgpd->get_kid(), SgpdContents::kKidSize);
  return true;
}

void ProcessPsshBoxes(Session* session,
                      const std::vector<const Box*>& pssh_boxes) {
  std::vector<uint8_t> concatenated_boxes;
  for (auto iter = pssh_boxes.begin(); iter != pssh_boxes.end(); ++iter) {
    const PsshContents* pssh =
        reinterpret_cast<const PsshContents*>((*iter)->get_contents());
    size_t pos = concatenated_boxes.size();
    concatenated_boxes.resize(pos + pssh->get_full_box().size());
    memcpy(&concatenated_boxes[pos],
           pssh->get_full_box().data(),
           pssh->get_full_box().size());
  }
  if (concatenated_boxes.size()) {
    session->is_encrypted_ = true;
    session->pssh_handler_(session->pssh_context_,
                           concatenated_boxes.data(),
                           concatenated_boxes.size());
  }
}

DashToHlsStatus ParseCenc(Session* dash_session, const Box* box) {
  const TencContents* tenc =
      reinterpret_cast<const TencContents*>(box->get_contents());
  if (!tenc) {
      // Must have TENC to parse CENC.
      return kDashToHlsStatus_BadConfiguration;
  }
  dash_session->default_iv_size_ = tenc->get_default_iv_size();
  memcpy(dash_session->key_id_, tenc->get_default_kid(),
         TencContents::kKidSize);

  const std::vector<const Box*> pssh_boxes =
      dash_session->parser_.FindDeepAll(BoxType::kBox_pssh);
  if (pssh_boxes.empty()) {
    // TENC is found in INIT Data, but PSSH has not been loaded.
    // Assumes clear lead.
    return kDashToHlsStatus_ClearContent;
  }

  if (!dash_session->pssh_handler_ || !dash_session->decryption_handler_) {
    DASH_LOG("Bad Configuration.", "Missing required CENC callback", "");
    return kDashToHlsStatus_BadConfiguration;
  }
  internal::ProcessPsshBoxes(dash_session, pssh_boxes);
  return kDashToHlsStatus_OK;
}

DashToHlsStatus ParseMediaType(Session* dash_session) {
  const Box* box = dash_session->parser_.FindDeep(BoxType::kBox_avcC);
  if (!box) {
    box = dash_session->parser_.FindDeep(BoxType::kBox_encv);
  }
  if (box) {
    dash_session->is_video_ = true;
    const AvcCContents *avcc =
    reinterpret_cast<const AvcCContents*>(box->get_contents());
    if (internal::ProcessAvcc(avcc, dash_session->stream_number_,
                              &dash_session->sps_pps_)
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
        DASH_LOG("Unknown Media Type", "Missing Audio/Video Box", "");
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
  return kDashToHlsStatus_OK;
}

DashToHlsStatus ParseTimeScaleAndSampleDuration(Session *dash_session) {
  const MvhdContents* mvhd = nullptr;
  const Box* box = dash_session->parser_.FindDeep(BoxType::kBox_mvhd);
  if (!box) {
    DASH_LOG("Bad Dash Content.", "No mvhd", "");
    return kDashToHlsStatus_BadDashContents;
  }
  mvhd = reinterpret_cast<const MvhdContents*>(box->get_contents());
  dash_session->timescale_ = mvhd->get_timescale();
  box = dash_session->parser_.FindDeep(BoxType::kBox_mdhd);
  if (box) {
    const MdhdContents* mdhd =
        reinterpret_cast<const MdhdContents*>(box->get_contents());
    dash_session->timescale_ = mdhd->get_timescale();
  }
  if (dash_session->timescale_ == 0) {
    DASH_LOG("Bad Dash Content.", "mvhd or mdhd needs a timescale.", "");
    return kDashToHlsStatus_BadDashContents;
  }

  box = dash_session->parser_.FindDeep(BoxType::kBox_trex);
  if (box) {
    const TrexContents* trex =
        reinterpret_cast<const TrexContents*>(box->get_contents());
    dash_session->trex_default_sample_duration_ =
        trex->get_default_sample_duration();
  }

  return kDashToHlsStatus_OK;
}
}  // namespace internal


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
  if (!(box && box->DoneParsing())) {
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

  if (internal::ParseTimeScaleAndSampleDuration(dash_session)
      != kDashToHlsStatus_OK) {
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
    if (internal::ProcessAvcc(avcc, dash_session->stream_number_,
                              &dash_session->sps_pps_)
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

  const std::vector<const Box*> pssh_boxes =
      dash_session->parser_.FindDeepAll(BoxType::kBox_pssh);
  if (pssh_boxes.empty()) {
    DASH_LOG("Missing boxes.", "Missing pssh box", "");
    return kDashToHlsStatus_BadConfiguration;
  }

  if (!dash_session->pssh_handler_ || !dash_session->decryption_handler_) {
    DASH_LOG("Bad Configuration.", "Missing required callback for CENC",
             "");
    return kDashToHlsStatus_BadConfiguration;
  }
  internal::ProcessPsshBoxes(dash_session, pssh_boxes);

  dash_session->default_iv_size_ = tenc->get_default_iv_size();
  memcpy(dash_session->key_id_, tenc->get_default_kid(),
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
    *tfdt =
        reinterpret_cast<const TfdtContents*>(boxes[index]->get_contents());
  }

  if (tfhd) {
    std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_tfhd);
    if (boxes.size() <= index) {
      DASH_LOG("Bad Dash Content.", "No tfhd", "");
      return kDashToHlsStatus_BadDashContents;
    }
    *tfhd =
        reinterpret_cast<const TfhdContents*>(boxes[index]->get_contents());
  }

  if (trun) {
    std::vector<const Box*> boxes = parser.FindDeepAll(BoxType::kBox_trun);
    if (boxes.size() <= index) {
      DASH_LOG("Bad Dash Content.", "No trun", "");
      return kDashToHlsStatus_BadDashContents;
    }
    *trun =
        reinterpret_cast<const TrunContents*>(boxes[index]->get_contents());
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
  if (session->default_iv_size_ > kIvSize) {
    DASH_LOG("Bad IV.",
             "Unexpected default_iv_size_ size.",
             "");
    return false;
  }
  uint8_t iv[kIvSize];
  memset(iv, 0, kIvSize);
  size_t size = saiz->get_sizes()[sample_number];
  if ((size != session->default_iv_size_) &&
      ((size - session->default_iv_size_ - sizeof(uint16_t)) %
           SaizContents::SaizRecordSize)) {
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
  *saio_position += session->default_iv_size_;
  size_t saio_records = 1;
  if (size > session->default_iv_size_) {
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
  if (mdat_offset > (uint64_t)SIZE_MAX) {
    DASH_LOG("Bad offset.", "mdat_offset too large.", "");
    return false;
  }
  size_t mdat_position = (size_t)mdat_offset;
  std::vector<uint8_t> encrypted_buffer;
  encrypted_buffer.resize(sample_size);
  // TODO(justsomeguy) don't walk off the end.
  for (size_t count = 0; count < saio_records; ++count) {
    size_t clear_bytes = 0;
    size_t encrypted_bytes = sample_size;
    if (size > session->default_iv_size_) {
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
    if (size > session->default_iv_size_) {
      *saio_position += SaizContents::SaizRecordSize;
    }
  }
  std::vector<uint8_t> clear_buffer;
  clear_buffer.resize(encrypted_position);
  if (encrypted_position > 0) {
    if (session->decryption_handler_(session->decryption_context_,
                                     &encrypted_buffer[0], &clear_buffer[0],
                                     encrypted_position, iv, sizeof(iv),
                                     key_id, nullptr, 0) !=
        kDashToHlsStatus_OK) {
      return false;
    }
  }
  out->resize(sample_size);

  // Putting things back should not be able to cause a buffer overflow.

  size_t decrypted_position = 0;
  encrypted_position = 0;
  if (mdat_offset > (uint64_t)SIZE_MAX) {
    DASH_LOG("Bad offset.", "mdat_offset too large.", "");
    return false;
  }
  mdat_position = (size_t)mdat_offset;
  for (size_t count = 0; count < saio_records; ++count) {
    size_t clear_bytes = 0;
    size_t encrypted_bytes = sample_size;
    if (size > session->default_iv_size_) {
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
                             uint64_t moof_number,
                             std::vector<uint8_t>* ts_output) {
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
  if (!dash_session->is_video_ && moof_number == 0) {
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
  uint32_t sample_number = 0;
  for (std::vector<TrunContents::TrackRun>::const_iterator
           iter = track_run.begin(); iter != track_run.end(); ++iter) {
    uint32_t sample_size = iter->sample_size_;

    if (!trun->IsSampleSizePresent()) {
      if (tfhd->IsDefaultSampleSizePresent()) {
        sample_size = tfhd->get_default_sample_size();
      }
    }
    uint64_t duration =
        (internal::GetDuration(trun, &(*iter), tfhd,
                               dash_session->trex_default_sample_duration_)
         * kDtsClock) / dash_session->timescale_;

    if (duration == 0) {
      return kDashToHlsStatus_BadDashContents;
    }
    uint64_t pts = dts;

    if (trun->IsSampleCompositionPresent()) {
      // Handle overflow of multiplying large 32 bit ints.
      int64_t offset = (iter->sample_composition_time_offset_ *
                        static_cast<int64_t>(kDtsClock) /
                        static_cast<int64_t>(dash_session->timescale_));
      pts += offset;
    }

    if (mdat_offset + sample_size > mdat->get_raw_data_length()) {
      DASH_LOG("Buffer overrun.", "Offset would be past the end of the mdat.",
               "");
      return kDashToHlsStatus_BadDashContents;
    }
    std::vector<uint8_t> decrypted;
    const uint8_t* key_id = nullptr;
    if (saio && saiz) {
      if (tenc) {
        key_id = tenc->get_default_kid();
      } else {
        key_id = dash_session->key_id_;
      }
    }

    // TODO: keep key IDs in vectors or find some other way to represent no ID
    uint8_t allZeros[16];
    memset(allZeros, 0, 16);

    if (key_id && memcmp(key_id, allZeros, 16)) {
      if (!DecryptSample(dash_session, sample_number, saiz, saio, key_id, mdat,
                         mdat_offset, sample_size, &saio_position,
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
        ts_out.ProcessSample(mdat_data + mdat_offset, sample_size,
                             dash_session->is_video_, sample_number == 0,
                             pts, dts, dts, duration, &output);
      } else {
        adts_out.ProcessSample(mdat_data + mdat_offset, sample_size,
                               &output);
      }
    }
    ++sample_number;
    ts_output->insert(ts_output->end(), output.begin(), output.end());
    mdat_offset += sample_size;
    dts += duration;
  }
  return kDashToHlsStatus_OK;
}
}  // namespace

extern "C" DashToHlsStatus
DashToHls_ParseSidx(DashToHlsSession* session, const uint8_t* bytes,
                    uint64_t length, DashToHlsIndex** index) {
  if (length > (uint64_t)SIZE_MAX) {
    DASH_LOG("Bad length.", "parse length too large.", "");
    return kDashToHlsStatus_BadConfiguration;
  }
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(bytes, (size_t)length) == 0) {
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
  if (length > (uint64_t)SIZE_MAX) {
    DASH_LOG("Bad length.", "parse length too large.", "");
    return kDashToHlsStatus_BadConfiguration;
  }
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(bytes, (size_t)length) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }

  // Check for CENC.
  const Box* box = dash_session->parser_.FindDeep(BoxType::kBox_tenc);
  if (!box) {
    return kDashToHlsStatus_ClearContent;
  }
  const std::vector<const Box*> pssh_boxes =
    dash_session->parser_.FindDeepAll(BoxType::kBox_pssh);
  if (pssh_boxes.empty()) {
    DASH_LOG("Missing boxes.", "Missing pssh box", "");
    return kDashToHlsStatus_BadConfiguration;
  }

  if (!dash_session->pssh_handler_ || !dash_session->decryption_handler_) {
    DASH_LOG("Bad Configuration.", "Missing required callback for CENC",
             "");
    return kDashToHlsStatus_BadConfiguration;
  }
  internal::ProcessPsshBoxes(dash_session, pssh_boxes);
  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
DashToHls_ParseSegmentPTS(struct DashToHlsSession* session,
                                       const uint8_t* bytes,
                                       uint64_t length,
                                       uint64_t* segment_pts,
                                       uint64_t* segment_duration) {
  if (length > (uint64_t)SIZE_MAX) {
    DASH_LOG("Bad length.", "parse length too large.", "");
    return kDashToHlsStatus_BadConfiguration;
  }
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(bytes, (size_t)length) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }

  DashToHlsStatus result =
      internal::ParseTimeScaleAndSampleDuration(dash_session);
  if (result != kDashToHlsStatus_OK) {
    return result;
  }

  const TfdtContents* tfdt = nullptr;
  const TfhdContents* tfhd = nullptr;
  const TrunContents* trun = nullptr;
  result = GetNeededBoxes(dash_session->is_encrypted_,
                          0,
                          dash_session->parser_,
                          NULL, NULL, &tfdt, &tfhd,
                          &trun, NULL, NULL, NULL);
  if (result != kDashToHlsStatus_OK) {
    return result;
  }

  *segment_pts = (tfdt->get_base_media_decode_time() * kDtsClock)
      / dash_session->timescale_;
  *segment_duration = 0;
  const std::vector<TrunContents::TrackRun>& track_run = trun->get_track_runs();
  for (std::vector<TrunContents::TrackRun>::const_iterator
           iter = track_run.begin(); iter != track_run.end(); ++iter) {
    uint64_t duration =
        (internal::GetDuration(trun, &(*iter), tfhd,
        dash_session->trex_default_sample_duration_) * kDtsClock)
            / dash_session->timescale_;
    *segment_duration += duration;
  }
  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
DashToHls_ParseLive(DashToHlsSession* session, const uint8_t* bytes,
                    uint64_t length,
                    uint32_t segment_number,
                    const uint8_t** hls_segment,
                    size_t* hls_length) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(bytes, (size_t)length) == 0) {
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
    if (internal::ProcessAvcc(avcc, dash_session->stream_number_,
                              &dash_session->sps_pps_)
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
                                          0, dash_session->parser_,
                                          &mdat, &moof, &tfdt, &tfhd,
                                          &trun, &saio, &saiz, &tenc);
  if (result != kDashToHlsStatus_OK) {
    return result;
  }

  result = internal::ParseTimeScaleAndSampleDuration(dash_session);
  if (result != kDashToHlsStatus_OK) {
    return result;
  }
  dash_session->output_[segment_number].clear();
  result = TransmuxToTS(dash_session, mdat, moof, tfdt,
                        tfhd, trun, saio, saiz, tenc, 0,
                        &dash_session->output_[segment_number]);
#ifdef USE_AVFRAMEWORK
  if (dash_session->encrypt_output_) {
    if (!Encrypt(reinterpret_cast<const DashToHlsSession*>(dash_session),
                 &dash_session->output_[segment_number])) {
      return kDashToHlsStatus_BadConfiguration;
    }
  }
#endif  // AVFRAMEWORK
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
  dash_session->output_[segment_number].clear();
  result = TransmuxToTS(dash_session, mdat,  moof, tfdt, tfhd,
                        trun, saio, saiz, tenc, 0,
                        &dash_session->output_[segment_number]);
#ifdef USE_AVFRAMEWORK
  if (dash_session->encrypt_output_) {
    if (!Encrypt(reinterpret_cast<const DashToHlsSession*>(dash_session),
                 &dash_session->output_[segment_number])) {
      return kDashToHlsStatus_BadConfiguration;
    }
  }
#endif  // AVFRAMEWORK
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
  const MdatContents* mdat = nullptr;
  const BoxContents* moof = nullptr;
  const TfdtContents* tfdt = nullptr;
  const TfhdContents* tfhd = nullptr;
  const TrunContents* trun = nullptr;
  const SaioContents* saio = nullptr;
  const SaizContents* saiz = nullptr;
  const TencContents* tenc = nullptr;

  Session* dash_session = reinterpret_cast<Session*>(session);
  DashParser parser;
  // The parser relies on offsets from the beginning of the file.
  parser.set_current_position(
      dash_session->index_.segments[segment_number].location);
  if (parser.Parse(dash_segment,
        (size_t)(dash_session->index_.segments[segment_number].length)) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }

  size_t segment_count = 0;
  dash_session->output_[segment_number].clear();
  while (true) {
    DashToHlsStatus result = GetNeededBoxes(dash_session->is_encrypted_,
                                            segment_count, parser,
                                            &mdat, &moof, &tfdt, &tfhd,
                                            &trun, &saio, &saiz, &tenc);
    if (result != kDashToHlsStatus_OK) {
      if (result == kDashToHlsStatus_NeedMoreData) {
        *hls_segment = &dash_session->output_[segment_number][0];
        *hls_length = dash_session->output_[segment_number].size();
        break;
      }
      return result;
    }

    result = TransmuxToTS(dash_session, mdat,  moof, tfdt, tfhd,
                          trun, saio, saiz, tenc,
                          segment_count,
                          &dash_session->output_[segment_number]);
    if (result != kDashToHlsStatus_OK) {
      return result;
    }
    ++segment_count;
  }
#ifdef USE_AVFRAMEWORK
  if (dash_session->encrypt_output_) {
    if (!Encrypt(reinterpret_cast<const DashToHlsSession*>(dash_session),
                 &dash_session->output_[segment_number])) {
      return kDashToHlsStatus_BadConfiguration;
    }
  }
#endif  // AVFRAMEWORK
  return kDashToHlsStatus_OK;
}

extern "C"
DashToHlsStatus DashToHls_ReleaseHlsSegment(DashToHlsSession* session,
                                            uint32_t hls_segment_number) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  // Only way to reset capacity is to swap.
  std::vector<uint8_t>().swap(dash_session->output_[hls_segment_number]);
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
void DashToHls_PrettyPrint(struct DashToHlsSession* session) {
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

// Test routine used to validate UDT does not crash on bad content.
extern "C" void
DashToHls_TestContent(unsigned char* content, size_t length) {
  Session* dash_session = new Session;
  dash_session->parser_.Parse(content, length);
  delete dash_session;
}

// Start UDT Routines -- Will replace DashToHls
extern "C" DashToHlsStatus
Udt_CreateSession(DashToHlsSession** session) {
  Session* dash_session = new Session;
  *session = reinterpret_cast<DashToHlsSession*>(dash_session);
  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
Udt_ReleaseSession(DashToHlsSession* session) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  delete dash_session;
  return kDashToHlsStatus_OK;
}

extern "C"
DashToHlsStatus Udt_ReleaseHlsSegment(DashToHlsSession* session,
                                      uint32_t hls_segment_number) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  // Only way to reset capacity is to swap.
  std::vector<uint8_t>().swap(dash_session->output_[hls_segment_number]);
  return kDashToHlsStatus_OK;
}

extern "C" DashToHlsStatus
Udt_ParseDash(DashToHlsSession* session,
              uint8_t stream_index,
              uint8_t* dash_data,
              size_t dash_data_size,
              uint8_t* pssh,
              size_t pssh_length,
              DashToHlsIndex** index) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  if (dash_session->parser_.Parse(dash_data, dash_data_size) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }
  dash_session->stream_number_ = stream_index;
  const Box* box = dash_session->parser_.Find(BoxType::kBox_sidx);
  if (box) {
    const SidxContents* sidx =
        reinterpret_cast<const SidxContents*>(box->get_contents());
    if (!sidx) {
      return kDashToHlsStatus_BadDashContents;
    }
    const std::vector<DashToHlsSegment>& locations(sidx->get_locations());
    dash_session->index_.index_count = static_cast<uint32_t>(locations.size());
    dash_session->index_.segments = &locations[0];
    if (index) {
      *index = &dash_session->index_;
    }
  }

  // Parse Timescale and Duration
  if (internal::ParseTimeScaleAndSampleDuration(dash_session) !=
          kDashToHlsStatus_OK) {
    return kDashToHlsStatus_BadDashContents;
  }

  // Check if content is Video or Audio
  if (internal::ParseMediaType(dash_session) != kDashToHlsStatus_OK) {
    return kDashToHlsStatus_BadDashContents;
  }

  // Check for CENC.
  box = dash_session->parser_.FindDeep(BoxType::kBox_tenc);
  if (!box) {
    return kDashToHlsStatus_ClearContent;
  }
  if (pssh) {
    const TencContents* tenc =
        reinterpret_cast<const TencContents*>(box->get_contents());
    dash_session->default_iv_size_ = tenc->get_default_iv_size();
    memcpy(dash_session->key_id_, tenc->get_default_kid(),
           TencContents::kKidSize);

    dash_session->is_encrypted_ = true;
    dash_session->pssh_handler_(dash_session->pssh_context_,
                               pssh,
                               pssh_length);
    return kDashToHlsStatus_OK;
  }
  return internal::ParseCenc(dash_session, box);
}

extern "C" DashToHlsStatus
Udt_ConvertDash(DashToHlsSession* session,
                uint32_t segment_number,
                const uint8_t* dash_data,
                size_t dash_data_size,
                const uint8_t** segment_out,
                size_t* segment_out_size) {
  const MdatContents* mdat = nullptr;
  const BoxContents* moof = nullptr;
  const SaioContents* saio = nullptr;
  const SaizContents* saiz = nullptr;
  const TencContents* tenc = nullptr;
  const TfdtContents* tfdt = nullptr;
  const TfhdContents* tfhd = nullptr;
  const TrunContents* trun = nullptr;

  Session* dash_session = reinterpret_cast<Session*>(session);
  DashParser parser;
  if (parser.Parse(dash_data, dash_data_size) == 0) {
    return kDashToHlsStatus_BadDashContents;
  }
  dash_session->encrypt_output_ = true;

  std::vector<const Box*> moof_boxes = parser.FindAll(BoxType::kBox_moof);
  dash_session->output_[segment_number].clear();
  size_t moof_count = 0;
  DashToHlsStatus result;
  while (moof_count < moof_boxes.size()) {
    result  = GetNeededBoxes(dash_session->is_encrypted_,
                             moof_count, parser,
                             &mdat, &moof, &tfdt, &tfhd,
                             &trun, &saio, &saiz, &tenc);

    if (moof) {
      const std::vector<const Box*> pssh_boxes =
          parser.FindDeepAll(BoxType::kBox_pssh);
      if (!pssh_boxes.empty()) {
        internal::ProcessPsshBoxes(dash_session, pssh_boxes);
        if (!internal::UpdateSessionKeyId(dash_session, parser)) {
          return kDashToHlsStatus_BadDashContents;
        }
      }
    }
    if (result != kDashToHlsStatus_OK) {
      return result;
    }
    result = TransmuxToTS(dash_session, mdat,  moof, tfdt, tfhd,
                          trun, saio, saiz, tenc,
                          moof_count,
                          &dash_session->output_[segment_number]);

    moof_count++;
  }
#ifdef USE_AVFRAMEWORK
  if (dash_session->encrypt_output_) {
    if (!Encrypt(reinterpret_cast<const DashToHlsSession*>(dash_session),
                 &dash_session->output_[segment_number])) {
      return kDashToHlsStatus_BadConfiguration;
    }
  }
#endif  // AVFRAMEWORK
  if (result == kDashToHlsStatus_OK) {
    *segment_out = &dash_session->output_[segment_number][0];
    *segment_out_size = dash_session->output_[segment_number].size();
  } else {
    return result;
  }
  return kDashToHlsStatus_OK;
}

extern "C"
void Udt_PrettyPrint(struct DashToHlsSession* session) {
  Session* dash_session = reinterpret_cast<Session*>(session);
  g_diagnostic_callback(dash_session->parser_.PrettyPrint("").c_str());
}
}  // namespace dash2hls

