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

LRUReplacer::LRUReplacer(size_t num_pages) : m_capacity{num_pages} {}

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
  std::lock_guard lg{m_mutex};
  if (m_frame_ids.empty()) {
    return false;
  }
  *frame_id = m_frame_ids.front();
  m_frame_ids.pop_front();
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  std::lock_guard lg{m_mutex};
  auto target = std::find(m_frame_ids.cbegin(), m_frame_ids.cend(), frame_id);
  if (target == m_frame_ids.end()) {
    return;
  }
  m_frame_ids.erase(target);
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  std::lock_guard lg{m_mutex};
  auto target = std::find(m_frame_ids.cbegin(), m_frame_ids.cend(), frame_id);
  if (target != m_frame_ids.end()) {
    return;
  }
  m_frame_ids.push_back(frame_id);
}

size_t LRUReplacer::Size() {
  std::lock_guard lg{m_mutex};
  return m_frame_ids.size();
}
}  // namespace bustub
