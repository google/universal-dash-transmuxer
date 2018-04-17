#include "library/ps/psm.h"

#include "library/dash/dash_parser.h"
#include "library/ps/pes.h"
#include "library/utilities.h"

#include <assert.h>

namespace {
const uint8_t kDefaultFlags1 = 0x80;
const uint8_t kDefaultFlags2 = 0x00;
const uint8_t kPesOptPts = 0x80;
const uint8_t kPesOptDts = 0x40;
const size_t kPesPtsLength = 5;
const size_t kPesDtsLength = 5;
const size_t kPesHeaderLength = 3;
const size_t kPesOptHeaderLength = 3;
const size_t kDefaultMaxSize = 65535;
const uint8_t kScramblingBitsMask = 0x03;
const uint8_t kScramblingBitsShift = 4;
const uint8_t kDataAlignementMask = 0x04;
const uint8_t kCopyrightBit = 0x02;
const uint8_t kOriginalBit = 0x01;

const uint8_t kPesNoOptHdrStreamIds[] = {
  0xBC,   // Program map
  0xBE,   // Padding
  0xBF,   // Private 2
  0xF0,   // ECM
  /* we store our V2 medatadata in stream_id 0xF1.  It may be encrypted, so
     we need the header extesion for scrambling bits.  This won't matter,
     since the extension could be considered part of the payload (follows the
     packet length)
     0xF1,   // EMM
  */
  0xF2,   // DSM CC
  0xF8,   // H.222.1 E
  0xFF    // Program stream directory
};
}  // namespace

namespace dash2hls {

const uint8_t PES::kPesStartCode[3] = {0x00, 0x00, 0x01};

PES::PES() : max_size_(kDefaultMaxSize), stream_id_(0),
             flags1_(kDefaultFlags1), flags2_(kDefaultFlags2), pts_(0),
             dts_(0), payload_(nullptr), payload_size_(0) {
}

bool PES::HasOptHeader() const {
  for (size_t count = 0; count < sizeof(kPesNoOptHdrStreamIds); ++count) {
    if (kPesNoOptHdrStreamIds[count] == stream_id_) {
      return false;
    }
  }
  return true;
}

// This routine has magic numbers still.  They were pulled from the Widevine
// code and should be replaced with meaningful constants.
// TODO(justsomeguy) look up magic numbers.
size_t PES::WriteHeader(uint8_t* buffer, size_t max_length) const {
  size_t size = GetHeaderSize();
  if (size > max_length) {
    return DashParser::kParseFailure;
  }

  memcpy(buffer, kPesStartCode, sizeof(kPesStartCode));
  buffer += sizeof(kPesStartCode);
  *buffer = stream_id_;
  ++buffer;
  // Packets more than 64K use 0 byte length. Technically, all video packets
  // could use size 0 with no harm but audio packets can't.
  size_t pes_size = GetSize() - sizeof(kPesStartCode) - kPesHeaderLength;
  if (pes_size > 0xFFFF) {
    buffer[0] = 0;
    buffer[1] = 0;
  } else {
    htonsToBuffer(pes_size, buffer);
  }
  buffer += 2;
  if (HasOptHeader()) {
    *buffer = flags1_;
    ++buffer;
    *buffer = flags2_;
    ++buffer;
    uint8_t* header_length_byte = buffer;
    *buffer = 0;
    ++buffer;
    if (flags2_ & kPesOptPts) {
      if (flags2_ & kPesOptDts) {
        *buffer = 0x31;
      } else {
        *buffer = 0x21;
      }
      *buffer |= (pts_ >> 29) & 0x0E;
      ++buffer;
      htonsToBuffer(((pts_ >> 14) & 0xFFFE) | 0x0001, buffer);
      buffer += 2;
      htonsToBuffer(((pts_ << 1) & 0xFFFE) | 0x0001, buffer);
      buffer += 2;
      *header_length_byte += kPesPtsLength;
    }
    if (flags2_ & kPesOptDts) {
      *buffer = 0x11 | ((dts_ >> 29) & 0x0E);
      ++buffer;
      htonsToBuffer(((dts_ >> 14) & 0xFFFE) | 0x0001, buffer);
      buffer += 2;
      htonsToBuffer(((dts_ << 1) & 0xFFFE) | 0x0001, buffer);
      //      buffer += 2;  // Commented to remove dead store warning.
      *header_length_byte += kPesDtsLength;
    }
  }
  return size;
}

size_t PES::GetSize() const {
  return GetHeaderSize() + payload_size_;
}

size_t PES::GetHeaderSize() const {
  size_t result = sizeof(kPesStartCode) + kPesHeaderLength;
  if (HasOptHeader()) {
    result += kPesOptHeaderLength;
    if (flags2_ & kPesOptPts) {
      result += kPesPtsLength;
    }
    if (flags2_ & kPesOptDts) {
      result += kPesDtsLength;
    }
  }
  return result;
}

size_t PES::GetFreePayloadBytes() const {
  return max_size_ - GetSize();
}

void PES::SetCopyright(bool copyright) {
  if (copyright) {
    flags1_ |= kCopyrightBit;
  } else {
    flags1_ &= ~kCopyrightBit;
  }
}

void PES::SetOriginal(bool original) {
  if (original) {
    flags1_ |= kOriginalBit;
  } else {
    flags1_ &= ~kOriginalBit;
  }
}

void PES::SetScramblingBits(uint8_t scramblingBits) {
  flags1_ &= ~(kScramblingBitsMask << kScramblingBitsShift);
  flags1_ |= (scramblingBits & kScramblingBitsMask) << kScramblingBitsShift;
}

void PES::SetDataAlignmentIndicator(bool aligned) {
  if (aligned) {
    flags1_ |= kDataAlignementMask;
  } else {
    flags1_ &= ~kDataAlignementMask;
  }
}

void PES::SetPts(uint64_t pts) {
  pts_ = pts;
  flags2_ |= kPesOptPts;
}

void PES::SetDts(uint64_t dts) {
  dts_ = dts;
  flags2_ |= kPesOptDts;
}

size_t PES::Write(uint8_t* buffer, size_t max_length) const {
  size_t size = GetSize();
  if (size > max_length) {
    DASH_LOG("PES Write failed", "PES::Write GetSize > max_length", nullptr);
    return DashParser::kParseFailure;
  }
  buffer += WriteHeader(buffer, max_length);
  memcpy(buffer, payload_, payload_size_);
  //  buffer += payload_size_;  // commented to remove the dead store warning
  return size;
}

size_t PES::WritePartial(uint8_t* buffer, size_t start, size_t length) const {
  size_t bytes_written = 0;
  if (start < GetHeaderSize()) {
    if (start + length < GetHeaderSize()) {
      DASH_LOG("PES WritePartial failed",
               "PES::WritePartial must write the entire header in one call.",
               nullptr);
      return DashParser::kParseFailure;
    }
    bytes_written = WriteHeader(buffer, length);
    start += bytes_written;
    length -= bytes_written;
    buffer += bytes_written;
  }
  // Assert that we aren't reading past the end of the payload:
  assert(start - GetHeaderSize() + length <= payload_size_);
  memcpy(buffer, payload_ + start - GetHeaderSize(), length);
  return bytes_written + length;
}
}  // namespace dash2hls
