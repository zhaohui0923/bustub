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
#include <cassert>

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages)
    :  // 构造与num_pages数量相同的值为std::list<frame_id_t>::iterator{}的元素
       // 表明没有任何的frame存在于frame_ids_链表中用以置换
      frame_id_to_iter_(num_pages) {}

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
  // 加锁实现线程安全
  std::lock_guard lg{latch_};
  // 没有任何可用于置换的frame的情况
  if (frame_ids_.empty()) {
    *frame_id = INVALID_PAGE_ID;
    return false;
  }
  // 有可用于置换的frame的情况
  // 将最前(最旧)的页面作为结果
  *frame_id = frame_ids_.front();
  // 将最前(最旧)的页面移出链表
  frame_ids_.pop_front();
  // 重置frame_id到链表iterator哈希查找表的对应项
  // 由于哈希表最大数量固定，且需要保持下标为frame_id
  // 故不是移除元素，而是使用哨兵值重置
  frame_id_to_iter_[*frame_id] = std::list<frame_id_t>::iterator{};
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  // Pin的语义为将frame移出replacer
  // 用以达到"固定"于内存，不进行置换的目的

  // 加锁实现线程安全
  std::lock_guard lg{latch_};
  // 给定的frame_id不在replacer中时
  // 无需处理，直接返回
  if (!IsInReplacer(frame_id)) {
    return;
  }
  // 使用哈希表快速的通过frame_id查找对应的链表iterator位置
  auto &it = frame_id_to_iter_[frame_id];
  // 在链表中删除iterator对应的元素
  frame_ids_.erase(it);
  // 重置frame_id到链表iterator哈希查找表的对应项
  // 由于哈希表最大数量固定，且需要保持下标为frame_id
  // 故不是移除元素，而是使用哨兵值重置
  it = std::list<frame_id_t>::iterator{};
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  // UnPin的语义为将frame加入到replacer中

  // 加锁用以实现线程安全
  std::lock_guard lg{latch_};

  // 如果该frame已经在replacer处于可以被置换的状态
  // 无需处理
  if (IsInReplacer(frame_id)) {
    return;
  }

  // 将最新的元素加入到链表的末尾
  frame_ids_.push_back(frame_id);
  // 更新frame_id到链表iterator哈希查找表的对应项
  frame_id_to_iter_[frame_id] = --frame_ids_.end();
}

size_t LRUReplacer::Size() {
  // 加锁用以实现线程安全
  std::lock_guard lg{latch_};
  // 返回当前可以用于被置换的frame的数量
  return frame_ids_.size();
}

bool LRUReplacer::IsInReplacer(frame_id_t frame_id) {
  return frame_id_to_iter_[frame_id] != std::list<frame_id_t>::iterator{};
}
}  // namespace bustub
