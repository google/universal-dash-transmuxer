// ProgramStreamOut takes mp4 samples and turns them into PS samples.
// These samples can then be wrapped in a TS layer.
//
// Expected usage is to call ProgramStreamOut::ProcessSample(in, out);
// All other routines are exposed for unit testing.
#ifndef _DASH2HLS_PROGRAM_STREAM_OUT_H_
#define _DASH2HLS_PROGRAM_STREAM_OUT_H_

#include <vector>

#include "include/DashToHlsApi.h"
#include "library/dash/box_type.h"
#include "library/dash/full_box_contents.h"
#include "library/ps/nalu.h"

namespace dash2hls {
  class SystemHeader;
  class PES;
  class PSM;

class ProgramStreamOut {
 public:
  ProgramStreamOut() : nalu_length_(0),
                       has_video_(false),
                       has_audio_(false) {
  }

  // TODO(justsomeguy) this interface requires an extra copy of input because
  // it's const.  See about doing it in place.
  void ProcessSample(const uint8_t* input, size_t input_length,
                     bool is_video,
                     bool is_sync_sample,
                     uint64_t pts, uint64_t dts, uint64_t scr,
                     uint64_t duration,
                     std::vector<uint8_t>* out);

  void set_nalu_length(size_t nalu_length) {nalu_length_ = nalu_length;}
  void set_sps_pps(const std::vector<uint8_t>& sps_pps) {sps_pps_ = sps_pps;}
  void set_audio_oid(const std::vector<uint8_t>& audio_oid) {
    audio_oid_ = audio_oid;
  }
  void set_has_video(bool flag) {has_video_ = flag;}
  void set_has_audio(bool flag) {has_audio_ = flag;}

 protected:
  void PreprocessNalus(std::vector<uint8_t>* buffer, bool* has_aud,
                       nalu::PicType* pic_type);
  void AddNeededNalu(std::vector<uint8_t>* buffer, nalu::PicType pic_type,
                     bool is_sync_sample);
  void ConvertLengthToStartCode(std::vector<uint8_t>* buffer);
  size_t WriteHeader(uint8_t* buffer, uint64_t scr, uint32_t mux_rate);
  void AddHeaders(const PES& pes, bool is_sync_sample, uint64_t duration,
                  uint64_t scr, std::vector<uint8_t>* out);
  size_t GetHeaderSize() const;
  size_t GetSize(const PES& pes) const;
  size_t GetSizeOfSyncPacket(bool is_sync_sample,
                           const SystemHeader& system_header,
                           const PSM& psm, const PES& pes) const;

 private:
  static const uint8_t kPackStartCode[4];
  size_t nalu_length_;
  std::vector<uint8_t> sps_pps_;
  std::vector<uint8_t> audio_oid_;
  bool has_video_;
  bool has_audio_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_PROGRAM_STREAM_OUT_H_
