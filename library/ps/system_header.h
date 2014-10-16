// Every segment starts with a system header.  See ISO-14966-10 for details
// on its internals.
#ifndef _DASH2HLS_SYSTEM_HEADER_H_
#define _DASH2HLS_SYSTEM_HEADER_H_

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

namespace dash2hls {

class SystemHeader {
 public:
  SystemHeader();

  uint32_t GetSize() const;
  uint32_t Write(uint8_t* buffer, uint32_t max_length) const;

  void SetRateBound(uint32_t rate_bound) {
    rate_bound_ = rate_bound;
  }
  void SetAudioBound(uint8_t audio_bound) {
    audio_bound_ = audio_bound;
  }
  void SetFixedFlag(bool fixedFlag);
  void SetCspsFlag(bool cspsFlag);
  void SetAudioLockFlag(bool audioLockFlag);
  void SetVideoLockFlag(bool videoLockFlag);
  void SetVideoBound(uint8_t video_bound) {
    video_bound_ = video_bound;
  }
  void SetPacketRestrictionFlag(bool flag);
  void AddStream(uint8_t stream_id);
  void SetBufferSizeBound(uint8_t stream_id, uint8_t buffer_size_scale,
                          uint16_t buffer_size_bound);

 private:
  uint32_t rate_bound_;
  uint8_t audio_bound_;
  uint8_t video_bound_;
  uint8_t flags1_;
  uint8_t flags2_;
  uint8_t flags3_;
  std::vector<std::pair<uint8_t, uint16_t> > streams_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_SYSTEM_HEADER_H_
