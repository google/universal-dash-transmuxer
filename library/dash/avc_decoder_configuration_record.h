// Video codec information.  Comes from either an mp4v or avcC box.  With Dash
// only the avcC box is ever seen.
// HLS only cares about the sps, pps, and nalu length.  The rest of the
// fields are parsed to help with diagnostics.
#ifndef _DASH2HLS_AVC_DECODER_CONFIGURATION_RECORD_H_
#define _DASH2HLS_AVC_DECODER_CONFIGURATION_RECORD_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace dash2hls {
class AvcDecoderConfigurationRecord {
 public:
  virtual ~AvcDecoderConfigurationRecord() {}
  const std::vector<std::vector<uint8_t> >&
      get_sequence_parameter_sets() const {return sequence_parameter_sets_;}
  const std::vector<std::vector<uint8_t> >&
      get_picture_parameter_sets() const {return picture_parameter_sets_;}
  size_t GetNaluLength() const {return length_size_minus_one_ + 1;}
  virtual size_t Parse(const uint8_t* buffer, size_t length);
  virtual std::string PrettyPrint(std::string indent) const;

 private:
  uint8_t version_;
  uint8_t avc_profile_indication_;
  uint8_t profile_compatibility_;
  uint8_t avc_level_indication_;
  uint8_t length_size_minus_one_;
  size_t sequence_parameter_sets_count_;
  std::vector<std::vector<uint8_t> > sequence_parameter_sets_;
  size_t picture_parameter_sets_count_;
  std::vector<std::vector<uint8_t> > picture_parameter_sets_;
};
}  // namespace dash2hls
#endif  // _DASH2HLS_AVC_DECODER_CONFIGURATION_RECORD_H_
