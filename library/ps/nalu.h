// Network Access Layer Unit (nalu) is used by H.264 to abstract samples,
// allowing greater flexibility in moving them from one format to another.
// For PS format the NALU need to be in start byte order instead of length
// order.  The filler NALU needs to be stripped and each chunk needs an Aud.
//
// Nalu can be either length encoded or start byte encoded.  Most of these
// routines work on length encoded as that's the input from mp4.  PS will
// want nalu in start byte format.
//
// See ISO 14496-10 for more information on NALU (section 7.3.1).
#ifndef _DASH2HLS_PS_NALU_H_
#define _DASH2HLS_PS_NALU_H_

#include <stdint.h>
#include <stdlib.h>

namespace dash2hls {
namespace nalu {
enum NaluType {
  kNaluType_Unspecified = 0,
  kNaluType_UnpartitionedNonIdrSlice = 1,
  kNaluType_SlicePartitionA = 2,
  kNaluType_SlicePartitionB = 3,
  kNaluType_SlicePartitionC = 4,
  kNaluType_IdrSlice = 5,
  kNaluType_SupplementalEnhancementInfo = 6,
  kNaluType_SequenceParamterSet = 7,
  kNaluType_PictureParameterSet = 8,
  kNaluType_AuDelimiter = 9,
  kNaluType_EndOfSequence = 10,
  kNaluType_EndOfStream = 11,
  kNaluType_Filler = 12,
  kNaluType_SpsExtension = 13,
  kNaluType_UnpartitionedAuxiliaryPictureSlice = 19,
  kNaluType_ScalalableExtensionNonIdrSlice = 20,
  kNaluType_ScalalableExtensionIdrSlice = 21
};

enum PicType {
  kPicType_Error = -1,
  kPicType_I = 0,
  kPicType_IP = 1,
  kPicType_IPB = 2,
  kPicType_SI = 3,
  kPicType_SI_SP = 4,
  kPicType_I_SI = 5,
  kPicType_I_SI_P_SP = 6,
  kPicType_I_SI_P_SP_B = 7
};

enum {
  kNaluTypeMask = 0x1f,
  kNaluHeaderSize = sizeof(uint32_t) + sizeof(uint8_t),
  kAudNaluSize = 2,
};

// A nalu starts with the nalu size in the first |nalu_length_size| bytes.
// In theory nalu_length_size can be any size but they should be 4.
// |nalu_buffer| is owned by the caller.
// |buffer_size| is the bytes in the buffer.
// |nalu_length_size| is the number of bytes used to encode the nalu length.
// Returns the payload size, so the entire nalu length is Length +
// nalu_length_size.  Returns 0 if there are not enough bytes in the buffer.
// A nalu of size 0 is an error as it would not have a type byte.
size_t GetLength(const uint8_t* nalu_buffer, size_t buffer_size,
                 size_t nalu_length_size);

// ReadBitsLength uses Exponential-Golomb coding to determine the length.
// Exp-Golomb allows a length that is not byte aligned.  |buffer| is a pointer
// to the byte stream and updated to the new location.  |buffer_size| is a
// pointer to how large the buffer is and is updated to the new buffer size.
// |bit_offset| is how many bits have already been processed and is updated.
// Returns the length or -1 on error.
//
// Example:  In this example there are two lengths in the nalu.
// const uint8_t* nalu = GetNalu();
// uint32_t buffer_size = GetNaluSize();
// uint32_t bit_offset = 0;
// size_t length = ReadBitsLength(&nalu, &buffer_size, &bit_offset);
int32_t ReadBitsLength(const uint8_t** buffer, uint32_t* buffer_size,
                       uint8_t* current_byte, uint32_t* bit_offset);

// GetSliceType returns the second variable length field after the nalu
// header or -1 on failure.
int32_t GetSliceType(const uint8_t* nalu_buffer, size_t buffer_size);

// PreprocessNalus removes any filler nalu packets, determines if |has_aud|
// and the |pic_type|.  |has_aud| is true if there is a nalu of type
// kNaluType_AuDelimiter.  |pic_type| is set by what frames are in the slices.
//
// Returns the new size of the buffer.  If the |buffer_size| does not match
// the lengths of the nalus in the |nalu_buffer| (too small or too large)
// then returns 0.
//
// See Length for description of |nalu_length_size|.
size_t PreprocessNalus(uint8_t* nalu_buffer, size_t buffer_size,
                       size_t nalu_length_size, bool* has_aud,
                       PicType* pic_type);

// Nalus can start with either a length or a start code.  PS and TS require
// start codes.  This code assumes a nalu_length_size of 4 bytes and requires
// the caller to validate that.  Returns false if the |buffer_size| is not
// large enough for all the nalus in |nalu_buffer|.
bool ReplaceLengthWithStartCode(uint8_t* nalu_buffer, size_t buffer_size);

}  // namespace nalu
}  // namespace dash2hls

#endif  // _DASH2HLS_PS_NALU_H_
