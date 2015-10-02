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

#ifndef DASHTOHLS_UTILITIES_H_
#define DASHTOHLS_UTILITIES_H_

#include <arpa/inet.h>
#include <string>
#include <utility>

namespace dash2hls {
// Network conversions that handle the casting correctly.
// There are many places in H.264 where various bytes need to be read or
// written to a buffer correctly.  In practice this conversion is very error
// prone and hard to read.  These helper routines are to guarantee correctness
// and improve readability in the code.
//
// The uint8_t* buffer is assumed to always be big enough to hold the
// appropriate value.
inline uint16_t ntohsFromBuffer(const uint8_t* buffer) {
  uint16_t networkBytes;
  memcpy(&networkBytes, buffer, sizeof(networkBytes));
  return ntohs(networkBytes);
}
inline void htonsToBuffer(uint16_t value, uint8_t* buffer) {
  uint16_t network_value = htons(value);
  memcpy(buffer, &network_value, sizeof(network_value));
}

inline uint32_t ntohlFromBuffer(const uint8_t* buffer) {
  uint32_t networkBytes;
  memcpy(&networkBytes, buffer, sizeof(networkBytes));
  return ntohl(networkBytes);
}
inline void htonlToBuffer(uint32_t value, uint8_t* buffer) {
  uint32_t network_value = htonl(value);
  memcpy(buffer, &network_value, sizeof(network_value));
}

uint64_t ntohllFromBuffer(const uint8_t* buffer);
void htonllToBuffer(uint64_t value, uint8_t* buffer);

// Determines if there are enough bytes left to continue parsing.
// Even though it's a trivial routine it documents the code by giving it an
// obvious name.  Also avoids size_t vs long comparison warnings.
inline bool EnoughBytesToParse(size_t parsed, size_t needed,
                               size_t total_length) {
  return parsed + needed <= total_length;
}

// Easy routines to print numbers.  Used by debugging routines.
std::string PrettyPrintValue(int8_t);
std::string PrettyPrintValue(int16_t);
std::string PrettyPrintValue(int32_t);
std::string PrettyPrintValue(int64_t);
std::string PrettyPrintValue(uint8_t);
std::string PrettyPrintValue(uint16_t);
std::string PrettyPrintValue(uint32_t);
std::string PrettyPrintValue(size_t);
std::string PrettyPrintValue(uint64_t);
std::string PrettyPrintBuffer(const uint8_t* buffer, size_t length);

// verbosity when debugging.
extern bool g_verbose_pretty_print;

std::string DumpMemory(const uint8_t* buffer, size_t length);

void DashToHlsDefaultDiagnosticCallback(const char* message);
extern "C" void (*g_diagnostic_callback)(const char* diagnostic_message);
void SendDashLog(const char* file, uint32_t line, const char* message,
                 const char* reason, const char* extra_fields);
#define DASH_LOG(message, reason, extra_fields) \
  SendDashLog(__FILE__, __LINE__, message, reason, extra_fields);

size_t wvcrc32(uint8_t* begin, int count);
size_t wvrunningcrc32(uint8_t* begin, int count, uint32_t crc);

}  // namespace dash2hls
#endif  // DASHTOHLS_UTILITIES_H_
