# NO LONGER MAINTAINED BY GOOGLE

See #19.  We apologize for any inconvenience.

-----

If you do not know H.264 then this code can be quite confusing.

### CONTRIBUTIONS

All non-googlers must sign a Contributing License Agreement (CLA).

### CUSTOMIZING

  Common.gypi must be customized.
  library/mac_test_files.mm needs to be populated with your content.
  DashToHlsTools.gyp needs to add paths, read the comments in that file.


### H.264 QUICK TUTORIAL.

H.264, or mpeg-4 is actually 15+ specs ranging from how to compress bits to
how to package a video for transport.  H.264 can also be placed in Mpeg-2
Program Streams (PS) and Transport Streams (TS).  Depending on the layer you
care about you should look at the different ISO 14496 specs.  Most of this
code is either 14496-10 or 14496-12.

Starting at the top Dash is in a 14496-12 container, which consists of boxes.
Each box starts with a length followed by a 4 byte code.  In this code all
objects are named after their code.  Any box may contain box specific data,
subboxes, or a combination of the two.  See 14496-12 for the exact definition
of any box.

All mpeg-4 files start with a ftyp box followed by a moov.  The ftyp is used
to verify the content can be played.  The moov contains ALL the information
needed to set up the codecs.

For any file with mpeg-4 there are several ways to find the data.  The DASH
content this code is targetted to uses a sidx box to specify segments.  Each
segment is a moof and an mdat.  The moof will have several important boxes in
it to specify the Samples.

Each Sample is a low level Mpeg-4 chunk of data made up of nalu, slices, and
other nitty gritty details.  See 14496-10 for a description of these.

The basic process is to find the DASH samples, massage the nalus, set up
Mpeg2 nalus for the codecs, then write out PS segments.  Take the PS segments
and pack them into TS segments.

### CLOCKS IN H.264

There are four clock pointers in H.264 delivered over Transport Stream.
System Clock Reference (SCR)
Program Clock Reference (PCR)
Decode Time Stamp (DTS)
Program Time Stamp (PTS)

The DTS and  PTS are all in 90KHz clock units.  So a value of 900,000 would be
10 seconds.  The SCR and PCR are in 27MHz clock units, so 10 seconds would be
900,000 * 300 = 270,000,000.

Each Sample must be delivered by either its SCR (PS) or PCR (TS).  Once the
packet is received it will be decoded by its DTS and put on the screen at
its PTS.  In H.264 packets are decoded out of order so while the DTS will go
up a constant amount the PTS will jump all over the place.

Some devices need an SCR/PCR before the DTS to give time for decoding after
the packets are guaranteed to arrive.  This is called buffer time.
While this library allows a buffer time it is expected to have a 0 buffer
time (SCR == DTS).
