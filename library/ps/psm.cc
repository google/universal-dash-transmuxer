#include "library/ps/psm.h"

#include <string.h>

#include "library/ps/pes.h"
#include "library/utilities.h"

namespace {
const size_t kBasePsmSize = 16;
const size_t kDescriptorOverheadSize = sizeof(uint32_t);
}  // namespace

namespace dash2hls {

const uint8_t PSM::kVideoAlignmentDescriptor[3] = {0x06, 0x01, 0x03};
const uint8_t PSM::kAudioAlignmentDescriptor[3] = {0x06, 0x01, 0x01};

PSM::PSM() : current_next_indicator_(true),
             psm_version_(0) {
}

size_t PSM::GetSize() const {
  size_t size = kBasePsmSize + descriptors_.size();
  for (std::vector<ElementaryStreamInfo>::const_iterator
           iter = elementary_streams_.begin();
       iter != elementary_streams_.end(); ++iter) {
    size += kDescriptorOverheadSize + iter->descriptors.size();
  }
  return size;
}

// This routine has magic numbers still.  They were pulled from the Widevine
// code and should be replaced with meaningful constants.
// TODO(justsomeguy) look up magic numbers.
size_t PSM::Write(uint8_t* buffer, uint32_t max_length) const {
  uint8_t* original_buffer = buffer;
  size_t size = GetSize();
  if (size > max_length) {
    return 0;
  }
  memcpy(buffer, PES::kPesStartCode, sizeof(PES::kPesStartCode));
  buffer += sizeof(PES::kPesStartCode);
  *buffer = 0xbc;
  ++buffer;
  htonsToBuffer(size - 6, buffer);
  buffer += sizeof(uint16_t);
  *buffer = 0x60 | (psm_version_ & 0x1f);
  if (current_next_indicator_) {
    *buffer |= 0x80;
  }
  ++buffer;
  *buffer = 0xff;
  ++buffer;
  htonsToBuffer(descriptors_.size(), buffer);
  buffer += sizeof(uint16_t);
  if (!descriptors_.empty()) {
    memcpy(buffer, descriptors_.data(), descriptors_.size());
    buffer += descriptors_.size();
  }
  // The rest of the size is the elemetary_streams_.
  htonsToBuffer(size - kBasePsmSize - descriptors_.size(), buffer);
  buffer += sizeof(uint16_t);
  for (std::vector<ElementaryStreamInfo>::const_iterator
           iter = elementary_streams_.begin();
       iter != elementary_streams_.end(); ++iter) {
    *buffer = iter->stream_type;
    ++buffer;
    *buffer = iter->stream_id;
    ++buffer;
    htonsToBuffer(iter->descriptors.size(), buffer);
    buffer += sizeof(uint16_t);
    if (!iter->descriptors.empty()) {
      memcpy(buffer, iter->descriptors.data(), iter->descriptors.size());
      buffer += iter->descriptors.size();
    }
  }
  htonlToBuffer(static_cast<uint32_t>(wvcrc32(original_buffer,
                                              static_cast<int32_t>(size - 4))),
                buffer);
  return size;
}

void PSM::AddDescriptor(const uint8_t* descriptor, uint16_t size) {
  size_t old_size = descriptors_.size();
  descriptors_.resize(old_size + size);
  memcpy(&descriptors_[old_size], descriptor, size);
}

void PSM::AddElementaryStream(uint8_t stream_id, const uint8_t stream_type) {
  ElementaryStreamInfo new_info;
  new_info.stream_id = stream_id;
  new_info.stream_type = stream_type;
  elementary_streams_.push_back(new_info);
}

void PSM::AddElementaryStreamDescriptor(uint8_t stream_id,
                                        const uint8_t* descriptor,
                                        uint16_t size) {
  for (std::vector<ElementaryStreamInfo>::iterator
           iter = elementary_streams_.begin();
       iter != elementary_streams_.end(); ++iter) {
    if (iter->stream_id == stream_id) {
      size_t old_size = iter->descriptors.size();
      iter->descriptors.resize(old_size + size);
      memcpy(&iter->descriptors[old_size], descriptor, size);
    }
  }
}
}  // namespace dash2hls
