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

#include "library/dash/box.h"
#include "library/dash/dash_parser.h"

namespace dash2hls {

DashParser::DashParser(): current_box_(0),
  current_stream_position_(0) {
}

size_t DashParser::AddToSpilloverIfNeeded(const uint8_t* buffer,
                                          size_t length) {
  if (!spillover_.empty()) {
    // Spillover is being used, copy the bytes.
    size_t bytes_spilled_over =
        current_box_->BytesNeededToContinue() - spillover_.size();
    if (bytes_spilled_over > length) {
      bytes_spilled_over = length;
    }
    size_t current_spillover_position = spillover_.size();
    spillover_.resize(current_spillover_position + bytes_spilled_over);
    memcpy(&spillover_[current_spillover_position], buffer,
           bytes_spilled_over);
    return bytes_spilled_over;
  }
  // Spillover not in use, don't do anything, let the caller use the
  // original buffer.
  return 0;
}

void DashParser::AddSpillover(const uint8_t* buffer, size_t length) {
  // If we continue with length 0, &spillover_[current_spillover_position] may
  // throw an exception because we're accessing the end of the vector with [].
  if (!length) return;

  size_t current_spillover_position = spillover_.size();
  spillover_.resize(current_spillover_position + length);
  memcpy(&spillover_[current_spillover_position], buffer, length);
}

size_t DashParser::Parse(const uint8_t* buffer, size_t length) {
  size_t position = AddToSpilloverIfNeeded(buffer, length);
  while (!spillover_.empty() || (position < length)) {
    if (!current_box_) {
      boxes_.push_back(Box(current_stream_position_));
      current_box_ = &boxes_.back();
    }
    size_t bytes_needed = current_box_->BytesNeededToContinue();
    if (spillover_.empty()) {
      if (position + bytes_needed <= length) {
        size_t bytes_parsed = current_box_->Parse(buffer + position,
                                                  bytes_needed);
        if (bytes_parsed != bytes_needed) {
          return kParseFailure;
        }
        position += bytes_parsed;
        current_stream_position_ += bytes_needed;
        if (current_box_->DoneParsing()) {
          current_box_ = 0;
        }
      } else {
        break;
      }
    } else {
      if (spillover_.size() < bytes_needed) {
        break;
      } else {
        size_t bytes_parsed = current_box_->Parse(&spillover_[0],
                                                  spillover_.size());
        if (bytes_parsed != spillover_.size()) {
          return kParseFailure;
        }
        current_stream_position_ += spillover_.size();
        // Only way to reset capacity is to swap.
        std::vector<uint8_t>().swap(spillover_);
        if (current_box_->DoneParsing()) {
          current_box_ = 0;
        }
      }
    }
  }
  AddSpillover(buffer + position, length - position);
  return length;
}

const Box* DashParser::Find(const BoxType::Type& box_type) const {
  for (std::vector<Box>::const_iterator iter = boxes_.begin();
       iter != boxes_.end(); ++iter) {
    if (iter->get_type().asUint32() == box_type) {
      return &(*iter);
    }
  }
  return nullptr;
}

const Box* DashParser::FindDeep(const BoxType::Type& box_type) const {
  for (std::vector<Box>::const_iterator iter = boxes_.begin();
       iter != boxes_.end(); ++iter) {
    if (iter->get_type().asUint32() == box_type) {
      return &(*iter);
    }
    // If there are contents and a dash_parser recurse in.
    if (!iter->get_contents()) {
      continue;
    }
    const DashParser* sub_parser = iter->get_contents()->get_dash_parser();
    if (!sub_parser) {
      continue;
    }
    const Box* found_box = sub_parser->FindDeep(box_type);
    if (found_box) {
      return found_box;
    }
  }
  return nullptr;
}

const std::vector<const Box*> DashParser::FindAll(
    const BoxType::Type& box_type) const {
  std::vector<const Box*> boxes;
  for (std::vector<Box>::const_iterator iter = boxes_.begin();
       iter != boxes_.end(); ++iter) {
    if (iter->get_type().asUint32() == box_type) {
      boxes.push_back(&(*iter));
    }
  }
  return boxes;
}

const std::vector<const Box*> DashParser::FindDeepAll(
    const BoxType::Type& box_type) const {
  std::vector<const Box*> boxes;
  for (std::vector<Box>::const_iterator iter = boxes_.begin();
       iter != boxes_.end(); ++iter) {
    if (iter->get_type().asUint32() == box_type) {
      boxes.push_back(&(*iter));
    }
    // If there are contents and a dash_parser recurse in.
    if (!iter->get_contents()) {
      continue;
    }
    const DashParser* sub_parser = iter->get_contents()->get_dash_parser();
    if (!sub_parser) {
      continue;
    }
    std::vector<const Box*> found_boxes = sub_parser->FindDeepAll(box_type);
    if (!found_boxes.empty()) {
      boxes.insert(boxes.end(), found_boxes.begin(), found_boxes.end());
    }
  }
  return boxes;
}

std::string DashParser::PrettyPrint(std::string indent) const {
  std::string result;
  for (std::vector<Box>::const_iterator iter = boxes_.begin();
       iter != boxes_.end(); ++iter) {
    result += indent + iter->PrettyPrint(indent + "  ") + "\n";
  }
  if (!result.empty()) {
    result.resize(result.size() - 1);
  }
  return result;
}
}  // namespace dash2hls
