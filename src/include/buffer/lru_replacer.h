//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.h
//
// Identification: src/include/buffer/lru_replacer.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <list>
#include <mutex>  // NOLINT
#include <vector>

#include "buffer/replacer.h"
#include "common/config.h"

namespace bustub {

/**
 * LRUReplacer implements the Least Recently Used replacement policy.
 */
class LRUReplacer : public Replacer {
 public:
  /**
   * Create a new LRUReplacer.
   * @param num_pages the maximum number of pages the LRUReplacer will be required to store
   */
  explicit LRUReplacer(size_t num_pages);

  /**
   * Destroys the LRUReplacer.
   */
  ~LRUReplacer() override;

  bool Victim(frame_id_t *frame_id) override;

  void Pin(frame_id_t frame_id) override;

  void Unpin(frame_id_t frame_id) override;

  size_t Size() override;

 private:
  // 判断指定的frame_id是否存在于replacer中
  // remark: 存在于replacer中的frame_id对应的frame都是可以被用于置换的
  bool IsInReplacer(frame_id_t frame_id);
  // 存在于replacer中的可用于被置换的frame的frame_id
  // 按照旧的在前，新的在后的顺序排列
  std::list<frame_id_t> frame_ids_;
  // 用于通过frame_id快速定位到链表迭代器位置的哈希表
  // 下标: 哈希表的key (frame_id)
  // 值: 对应链表迭代器位置，当指定的key (frame_id)不存在于replacer中时，为std::list<frame_id_t>::iterator{}哨兵值
  std::vector<std::list<frame_id_t>::iterator> frame_id_to_iter_;
  // 用于实现线程安全的锁
  std::mutex latch_;
};

}  // namespace bustub
