// ElementaryStreamDescriptor contain all information needed by the codec
// to decode the samples that follow.  The only really relevant information
// needed for is the DecodeSpecificInformation, namely the
// audio_config_ found in mp4a boxes.  In theory DASH could use
// mp4v boxes also but they tend to use avc1 instead, sp these boxes should
// be seen in only audio tracks.
//
// See ISO 14496-1 for more information (section 7.2.6.5).
#ifndef _DASH2HLS_DASH_ELEMENTARY_STREAM_DESCRIPTOR_H_
#define _DASH2HLS_DASH_ELEMENTARY_STREAM_DESCRIPTOR_H_

#include <string>
#include <vector>

#include "library/dash/base_descriptor.h"
#include "library/dash/decoder_descriptor.h"

namespace dash2hls {

class ElementaryStreamDescriptor : public BaseDescriptor {
 public:
  virtual std::string PrettyPrint(std::string indent) const;
  virtual size_t Parse(const uint8_t* buffer, size_t length);

  bool HasStreamDependsFlag() const {
    return flags_ & kStreamDependenceFlagMask;
  }

  bool HasUrl() const {return flags_ & kUrlFlagMask;}
  bool HasOcrStream() const {return flags_ & kOcrStreamFlagMask;}
  uint8_t StreamPriority() const {return flags_ & kStreamPriorityMask;}

  uint8_t get_audio_object_type() const {
    return decoder_descriptor_.get_audio_object_type();
  }

  uint8_t get_sampling_frequency_index() const {
    return decoder_descriptor_.get_sampling_frequency_index();
  }

  uint8_t get_channel_config() const {
    return decoder_descriptor_.get_channel_config();
  }
  const std::vector<uint8_t>& get_audio_config() const {
    return decoder_descriptor_.get_audio_config();
  }

 private:
  enum {
    kStreamDependenceFlagMask = 0x80,
    kUrlFlagMask = 0x40,
    kOcrStreamFlagMask = 0x20,
    kStreamPriorityMask = 0x1f,
  };
  uint16_t id_;
  uint8_t flags_;
  uint8_t depends_on_id_;  // Misleading name but that is what it's called.
  std::vector<uint8_t> url_;
  uint16_t ocr_id_;
  DecoderDescriptor decoder_descriptor_;

  // The following descriptors are not needed for Dash.
  BaseDescriptor sl_descriptor_;
  BaseDescriptor ipi_ptr_;
  BaseDescriptor language_;
  BaseDescriptor qos_;
  BaseDescriptor registration_;
  BaseDescriptor extension_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_DASH_ELEMENTARY_STREAM_DESCRIPTOR_H_
