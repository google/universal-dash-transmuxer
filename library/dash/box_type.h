#ifndef _DASH2HLS_BOX_TYPE_H_
#define _DASH2HLS_BOX_TYPE_H_

/*
Copyright 2014 Google Inc. All rights reserved.

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

// All mp4/dash files are made up of boxes.  Each box starts with a 4 byte
// code describing the box.  The 4 bytes in network byte order are a human
// readable abbreviation of the type of box, for example, moov is a Movie box.
//
// BoxType is a simple class to detect the box type and Pretty print the box
// type during debugging.

#include <stdint.h>
#include <string.h>
#include <string>

#include "library/utilities.h"

namespace dash2hls {

class BoxType {
 public:
  // Mpeg box types.  The hex value is ntohl of the 4 char name of the box.
  // This list is not comprehensive but contains the most likely found boxes.
  enum Type {
    kBox_avc1 = 'avc1',
    kBox_avcC = 'avcC',
    kBox_cbmp = 'cbmp',
    kBox_dinf = 'dinf',
    kBox_edts = 'edts',
    kBox_elst = 'elst',
    kBox_emsg = 'emsg',
    kBox_enca = 'enca',
    kBox_encv = 'encv',
    kBox_equi = 'equi',
    kBox_esds = 'esds',
    kBox_ftyp = 'ftyp',
    kBox_hdlr = 'hdlr',
    kBox_mdat = 'mdat',
    kBox_mdhd = 'mdhd',
    kBox_mdia = 'mdia',
    kBox_mfhd = 'mfhd',
    kBox_minf = 'minf',
    kBox_moof = 'moof',
    kBox_moov = 'moov',
    kBox_mp4a = 'mp4a',
    kBox_mshp = 'mshp',
    kBox_mvex = 'mvex',
    kBox_mvhd = 'mvhd',
    kBox_prft = 'prft',
    kBox_prhd = 'prhd',
    kBox_proj = 'proj',
    kBox_pssh = 'pssh',
    kBox_saio = 'saio',
    kBox_saiz = 'saiz',
    kBox_sbgp = 'sbgp',
    kBox_schi = 'schi',
    kBox_sgpd = 'sgpd',
    kBox_sidx = 'sidx',
    kBox_sinf = 'sinf',
    kBox_smhd = 'smhd',
    kBox_st3d = 'st3d',
    kBox_stbl = 'stbl',
    kBox_stco = 'stco',
    kBox_stsc = 'stsc',
    kBox_stsd = 'stsd',
    kBox_stss = 'stss',
    kBox_stsz = 'stsz',
    kBox_stts = 'stts',
    kBox_sv3d = 'sv3d',
    kBox_tenc = 'tenc',
    kBox_tfdt = 'tfdt',
    kBox_tfhd = 'tfhd',
    kBox_tkhd = 'tkhd',
    kBox_traf = 'traf',
    kBox_trak = 'trak',
    kBox_trex = 'trex',
    kBox_trun = 'trun',
    kBox_vmhd = 'vmhd',
    kBox_ytmp = 'ytmp',
    kBox_NoNe = '....',  // Not a real box, useful for testing.
  };

 public:
  BoxType() {
    memset(box_type_, 0, sizeof(box_type_));
  }

  explicit BoxType(Type type) {
    set_type(type);
  }

  // Sets the box_type_ according to host byte order.
  void set_type(Type type);
  // Sets the box_type_ from an array of network ordered bytes.
  void set_type(const uint8_t* type);

  // The host byte order value of box_type_.
  uint32_t asUint32() const;

  // The network byte order value of the box_type_.
  uint32_t asBoxType() const;

  // Debugging routine to print diagnostic information.
  std::string PrettyPrint(std::string indent) const;

 private:
  char box_type_[4];
};

}  // namespace dash2hls

#endif  // DASH2HLS_BOX_TYPE_H_
