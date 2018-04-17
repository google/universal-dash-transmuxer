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

#ifndef DASHTOHLS_UTILITIES_GMOCK_H_
#define DASHTOHLS_UTILITIES_GMOCK_H_

#include <gmock/gmock.h>
#include <utility>
#include <vector>

namespace testing {
namespace internal {
template <typename T>
class MemEqMatcher {
 public:
  enum {
    kDefaultBytesToPrint = 8,
  };
  MemEqMatcher(const T* expected, size_t expected_length)
      : expected_(expected, expected + expected_length),
        max_bytes_to_print_(kDefaultBytesToPrint) {
  }

  MemEqMatcher(T* expected, size_t expected_length, size_t max_bytes_to_print)
      : expected_(expected, expected + expected_length),
        max_bytes_to_print_(max_bytes_to_print) {
  }

  void DescribeTo(::std::ostream* os) const {
    *os << "a " << expected_.size() << " byte memory block ";
    DumpStartOfRange(os, &expected_[0], expected_.size());
  }
  void DescribeNegationTo(::std::ostream* os) const {
    *os << "not a " << expected_.size()
        << " byte memory block ";
    DumpStartOfRange(os, &expected_[0], expected_.size());
  }
  template <typename Iterator>
  void DumpStartOfRange(::std::ostream* os, Iterator bytes,
                        size_t length) const {
    size_t bytes_to_print = length;
    if (bytes_to_print > max_bytes_to_print_) {
      *os << "starting with ";
      bytes_to_print = max_bytes_to_print_;
    } else {
      *os << "containing ";
    }
    for (size_t count = 0; count < bytes_to_print; ++count) {
      *os << std::hex << uint32_t(*bytes);
      if (count + 1 < bytes_to_print) {
        *os << ", ";
      }
      ++bytes;
    }
    *os << std::dec;
  }
  template <typename Pointer>
  bool MatchAndExplain(std::pair<Pointer*, size_t>actual,
                       MatchResultListener* listener) const {
    if (expected_.size() != actual.second) {
      if (listener->stream()) {
        *listener << "a " << actual.second <<
            " byte memory block ";
        DumpStartOfRange(listener->stream(), actual.first, actual.second);
      }
      return false;
    }
    for (size_t count = 0; count < expected_.size(); ++count) {
      if (actual.first[count] != expected_[count]) {
        if (listener->stream()) {
          *listener << "a " << actual.second << " byte memory block ";
          DumpStartOfRange(listener->stream(), actual.first, actual.second);
          *listener << " and at byte " << count << " has a value of "
                    << std::hex << uint32_t(actual.first[count])
                    << " instead of " << uint32_t(expected_[count])
                    << std::dec;
        }
        return false;
      }
    }
    return true;
  }

 private:
  const std::vector<T> expected_;
  const uint32_t max_bytes_to_print_;
};
}  // namespace internal

template <typename T>
PolymorphicMatcher<internal::MemEqMatcher<T> > MemEq(
    const T* expected, size_t expected_length) {
  return MakePolymorphicMatcher(internal::MemEqMatcher<T>(expected,
                                                          expected_length));
}

template <typename T>
PolymorphicMatcher<internal::MemEqMatcher<T> > MemEq(
    const T* expected, size_t expected_length, size_t max_bytes_to_print) {
  return MakePolymorphicMatcher(internal::MemEqMatcher<T>(expected,
                                                          expected_length,
                                                          max_bytes_to_print));
}
}  // namespace testing
#endif  // DASHTOHLS_UTILITIES_GMOCK_H_
