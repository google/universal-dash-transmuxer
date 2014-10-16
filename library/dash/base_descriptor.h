// Descriptors in H.264 are used to describe configurations inside of
// various boxes.  For DASH they tend to only be used in audio mp4a boxes.
//
// See ISO 14496-1 for more information (section 8.3.3).
#ifndef _DASH2HLS_DASH_BASE_DESCRIPTOR_H_
#define _DASH2HLS_DASH_BASE_DESCRIPTOR_H_

#include <stdint.h>
#include <string>

namespace dash2hls {
class BaseDescriptor {
 public:
  enum Tag {
    kForbiddenTag = 0x00,
    kObjectDescrTag = 0x01,
    kInitialObjectDescrTag = 0x02,
    kES_DescrTag = 0x03,
    kDecoderConfigDescrTag = 0x04,
    kDecSpecificInfoTag = 0x05,
    kSLConfigDescrTag= 0x06,
    kContentIdentDescrTag = 0x07,
    kSupplContentIdentDescrTag = 0x08,
    kIPI_DescrPointerTag = 0x09,
    kIPMP_DescrPointerTag = 0x0A,
    kIPMP_DescrTag = 0x0B,
    kQoS_DescrTag = 0x0C,
    kRegistrationDescrTag = 0x0D,
    kES_ID_IncTag = 0x0E,
    kES_ID_RefTag= 0x0F,
    kMP4_IOD_Tag = 0x10,
    kMP4_OD_Tag = 0x11,
    kIPL_DescrPointerRefTag = 0x12,
    kExtensionProfileLevelDescrTag = 0x13,
    kprofileLevelIndicationIndexDescrTag = 0x14,
    // 0x15-0x3F Reserved for ISO use.
    kContentClassificationDescrTag = 0x40,
    kKeyWordDescrTag = 0x41,
    kRatingDescrTag = 0x42,
    kLanguageDescrTag = 0x43,
    kShortTextualDescrTag = 0x44,
    kExpandedTextualDescrTag = 0x45,
    kContentCreatorNameDescrTag = 0x46,
    kContentCreationDateDescrTag = 0x47,
    kOCICreatorNameDescrTag = 0x48,
    kOCICreationDateDescrTag = 0x49,
    kSmpteCameraPositionDescrTag = 0x4A,
    kSegmentDescrTag = 0x4B,
    kMediaTimeDescrTag = 0x4C,
    //    0x4D-0x5F Reserved for ISO use (OCI extensions).
    kIPMP_ToolsListDescrTag = 0x60,
    kIPMP_ToolTag = 0x61,
    kM4MuxTimingDescrTag = 0x62,
    kM4MuxCodeTableDescrTag = 0x63,
    kExtSLConfigDescrTag = 0x64,
    kM4MuxBufferSizeDescrTag = 0x65,
    kM4MuxIdentDescrTag = 0x66,
    kDependencyPointerTag = 0x67,
    kDependencyMarkerTag = 0x68,
    kM4MuxChannelDescrTag = 0x69,
    // 0x6A-0xBF Reserved for ISO use.
    // 0xC0-0xFE User private.
    kForbiddenLastTag = 0xFF,
  };

  BaseDescriptor() : tag_(kForbiddenTag), size_(0) {}
  virtual ~BaseDescriptor() {
  }

  Tag get_tag() {return tag_;}
  size_t get_size() {return size_;}

  // Returns the size of the header and populates the tag_ and size_.
  size_t ParseHeader(const uint8_t* buffer, size_t length);

  // Subclasses should not call Parse.  Parse assumes there is no
  // subclass and just eats the entire content.
  virtual size_t Parse(const uint8_t* buffer, size_t length);
  virtual std::string PrettyPrint(std::string indent) const;

 private:
  Tag tag_;
  size_t size_;
};
}  // namespace dash2hls

#endif  // _DASH2HLS_DASH_BASE_DESCRIPTOR_H_
