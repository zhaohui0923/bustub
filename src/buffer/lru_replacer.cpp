//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.cpp
//
// Identification: src/buffer/lru_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_replacer.h"
#include <algorithm>

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) : capacity_{num_pages} {}

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
  std::lock_guard lg{latch_};
  if (frame_ids_.empty()) {
    return false;
  }
  *frame_id = frame_ids_.front();
  frame_ids_.pop_front();
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  std::lock_guard lg{latch_};
  auto target = std::find(frame_ids_.cbegin(), frame_ids_.cend(), frame_id);
  if (target == frame_ids_.end()) {
    return;
  }
  frame_ids_.erase(target);
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  std::lock_guard lg{latch_};
  auto target = std::find(frame_ids_.cbegin(), frame_ids_.cend(), frame_id);
  if (target != frame_ids_.end()) {
    return;
  }
  frame_ids_.push_back(frame_id);
}

size_t LRUReplacer::Size() {
  std::lock_guard lg{latch_};
  return frame_ids_.size();
}
}  // namespace bustub
