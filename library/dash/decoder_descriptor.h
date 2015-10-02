// The DecodeDescriptor contains quite a bit of interesting information that
// is not needed for HLS.  It does contain the DecoderSpecificInformation,
// which has the necessary audio configuration.  Pretty much everything else is
// only for diagnostics.
//
// See ISO 14496-1 for more information (section 7.2.6.6.1).
#ifndef _DASH2HLS_DASH_DECODER_DESCRIPTOR_H_
#define _DASH2HLS_DASH_DECODER_DESCRIPTOR_H_

#include <string>
#include <vector>

#include "library/dash/base_descriptor.h"

namespace dash2hls {

class DecoderDescriptor : public BaseDescriptor {
 public:
  virtual size_t Parse(const uint8_t* buffer, size_t length);
  virtual std::string PrettyPrint(std::string indent) const;

  // Framing needs these values, which are parsed out of the audio_config_.
  uint8_t get_audio_object_type() const {
    return decoder_specific_info_.get_audio_object_type();
  }

  uint8_t get_sampling_frequency_index() const {
    return decoder_specific_info_.get_sampling_frequency_index();
  }

  uint32_t get_extension_sampling_frequency() const {
    return decoder_specific_info_.get_extension_sampling_frequency();
  }

  uint8_t get_channel_config() const {
    return decoder_specific_info_.get_channel_config();
  }

  // The PMT needs the raw audio_config_.
  const std::vector<uint8_t>& get_audio_config() const {
    return decoder_specific_info_.get_audio_config();
  }

  bool sbr_present() const {
    return decoder_specific_info_.sbr_present();
  }

 private:
  class DecoderSpecificInfo : public BaseDescriptor {
   public:
    virtual size_t Parse(const uint8_t* buffer, size_t length);
    virtual std::string PrettyPrint(std::string indent) const;
    uint8_t get_audio_object_type() const {return audio_object_type_;}
    uint8_t get_sampling_frequency_index() const {
      return sampling_frequency_index_;
    }
    uint32_t get_extension_sampling_frequency() const;
    uint8_t get_channel_config() const {return channel_config_;}
    const std::vector<uint8_t>& get_audio_config() const {return audio_config_;}
    bool sbr_present() const {return sbr_present_;}

   private:
    uint8_t audio_object_type_;
    uint8_t sampling_frequency_index_;
    uint8_t channel_config_;
    uint8_t extension_audio_object_type_ = 0;
    uint8_t extension_sampling_frequency_index_ = 0;
    std::vector<uint8_t> audio_config_;
    bool sbr_present_ = false;
  };

  uint8_t objectTypeIndication_;
  uint8_t stream_type_;
  bool upstream_;
  uint32_t buffer_size_db_;
  uint32_t max_bitrate_;
  uint32_t average_bitrate_;
  DecoderSpecificInfo decoder_specific_info_;
  BaseDescriptor profile_descriptor_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_DASH_DECODER_DESCRIPTOR_H_
