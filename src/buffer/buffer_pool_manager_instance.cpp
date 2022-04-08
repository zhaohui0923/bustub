//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager_instance.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager_instance.h"

#include "common/macros.h"

namespace bustub {

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, DiskManager *disk_manager,
                                                     LogManager *log_manager)
    : BufferPoolManagerInstance(pool_size, 1, 0, disk_manager, log_manager) {}

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, uint32_t num_instances, uint32_t instance_index,
                                                     DiskManager *disk_manager, LogManager *log_manager)
    : pool_size_(pool_size),
      num_instances_(num_instances),
      instance_index_(instance_index),
      next_page_id_(instance_index),
      disk_manager_(disk_manager),
      log_manager_(log_manager) {
  BUSTUB_ASSERT(num_instances > 0, "If BPI is not part of a pool, then the pool size should just be 1");
  BUSTUB_ASSERT(
      instance_index < num_instances,
      "BPI index cannot be greater than the number of BPIs in the pool. In non-parallel case, index should just be 1.");
  // We allocate a consecutive memory space for the buffer pool.
  pages_ = new Page[pool_size_];
  replacer_ = new LRUReplacer(pool_size);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManagerInstance::~BufferPoolManagerInstance() {
  delete[] pages_;
  delete replacer_;
}

bool BufferPoolManagerInstance::FlushPgImp(page_id_t page_id) {
  // Make sure you call DiskManager::WritePage!
  // 加锁用以保证线程安全
  std::lock_guard<std::mutex> lg{latch_};

  // 如果是INVALID_PAGE_ID的情况
  // 那么后面应该通过哈希表无法找到
  // 故无需特殊处理
#if 0
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
#endif

  // 在本buffer pool中查找指定page_id的frame
  auto it = page_table_.find(page_id);
  // 如果没有找到直接返回即可
  if (it == page_table_.end()) {
    return false;
  }
  // 如果找到,记录frame_id
  auto frame_id = it->second;
  // 并通过frame_id定位到page
  auto *page = &(pages_[frame_id]);
  // 将该page写入到磁盘
  disk_manager_->WritePage(page_id, page->data_);
  // 重置dirty的标志位
  page->is_dirty_ = false;
  return true;
}

void BufferPoolManagerInstance::FlushAllPgsImp() {
  // You can do it!
  // 加锁用以实现线程安全
  std::lock_guard<std::mutex> lg{latch_};
  // 遍历page_table中的每一项
  for (auto pt : page_table_) {
    // 此处不调用FlushPgImp
    // 因此此处遍历到的项都必然在page_table中
    // 若使用FlushPgImpl会有不必要的重复判断，降低效率
    auto page_id = pt.first;
    auto frame_id = pt.second;
    auto *page = &(pages_[frame_id]);
    disk_manager_->WritePage(page_id, page->GetData());
    page->is_dirty_ = false;
  }
}

Page *BufferPoolManagerInstance::NewPgImp(page_id_t *page_id) {
  // 0.   Make sure you call AllocatePage!
  // 1.   If all the pages in the buffer pool are pinned, return nullptr.
  // 2.   Pick a victim page P from either the free list or the replacer. Always pick from the free list first.
  // 3.   Update P's metadata, zero out memory and add P to the page table.
  // 4.   Set the page ID output parameter. Return a pointer to P.
  // 加锁用以实现线程安全
  std::lock_guard<std::mutex> lg{latch_};
  auto frame_id = frame_id_t{0};
  Page *page = nullptr;
  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
    page = &(pages_[frame_id]);
  } else if (replacer_->Victim(&frame_id)) {
    page = &(pages_[frame_id]);
    if (page->IsDirty()) {
      disk_manager_->WritePage(page->GetPageId(), page->GetData());
    }
    page_table_.erase(page->GetPageId());
  } else {
    return nullptr;
  }

  page->page_id_ = AllocatePage();
  page->is_dirty_ = false;
  page->pin_count_ = 1;
  page->ResetMemory();

  page_table_[page->GetPageId()] = frame_id;
  *page_id = page->GetPageId();
  // 此时无需调用replacer的Pin方法
  // replacer的Pin方法语义为将frame移出
  // 而此frame的来源
  // 1. free_list，必然不在replacer中
  // 2. replacer置换出，必然不在replacer中
  // replacer_->Pin(frame_id);

  return page;
}

Page *BufferPoolManagerInstance::FetchPgImp(page_id_t page_id) {
  // 1.     Search the page table for the requested page (P).
  // 1.1    If P exists, pin it and return it immediately.
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  // 2.     If R is dirty, write it back to the disk.
  // 3.     Delete R from the page table and insert P.
  // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.
  std::lock_guard<std::mutex> lock_guard(latch_);
  frame_id_t frame_id{0};
  Page *page = nullptr;

  auto item = page_table_.find(page_id);
  if (item != page_table_.end()) {
    frame_id = item->second;
    page = &pages_[frame_id];
    page->pin_count_++;
    replacer_->Pin(frame_id);
    return page;
  }

  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
    page = &pages_[frame_id];
  } else if (replacer_->Victim(&frame_id)) {
    page = &pages_[frame_id];
    if (page->IsDirty()) {
      disk_manager_->WritePage(page->GetPageId(), page->GetData());
    }
    page_table_.erase(page->GetPageId());
  } else {
    return nullptr;
  }

  page->page_id_ = page_id;
  page->pin_count_ = 1;
  page->is_dirty_ = false;
  disk_manager_->ReadPage(page->GetPageId(), page->GetData());
  page_table_[page->GetPageId()] = frame_id;
  replacer_->Pin(frame_id);

  return page;
}

bool BufferPoolManagerInstance::DeletePgImp(page_id_t page_id) {
  // 0.   Make sure you call DeallocatePage!
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  // 2.   If P exists, but has a non-zero pin-count, return false. Someone is using the page.
  // 3.   Otherwise, P can be deleted. Remove P from the page table, reset its metadata and return it to the free list.
  std::lock_guard<std::mutex> lg{latch_};
  DeallocatePage(page_id);
  auto it_page_table = page_table_.find(page_id);
  if (it_page_table == page_table_.end()) {
    return true;
  }
  auto frame_id = it_page_table->second;
  Page *page = &(pages_[frame_id]);
  if (page->pin_count_ != 0) {
    return false;
  }

  if (page->IsDirty()) {
    disk_manager_->WritePage(page->GetPageId(), page->GetData());
  }

  replacer_->Pin(frame_id);
  page_table_.erase(it_page_table);
  page->pin_count_ = 0;
  page->is_dirty_ = false;
  page->ResetMemory();
  page->page_id_ = INVALID_PAGE_ID;
  free_list_.push_back(frame_id);
  return true;
}

/**
 * Unpin the target page from the buffer pool.
 * @param page_id id of page to be unpinned
 * @param is_dirty true if the page should be marked as dirty, false otherwise
 * @return false if the page pin count is <= 0 before this call, true otherwise
 */
bool BufferPoolManagerInstance::UnpinPgImp(page_id_t page_id, bool is_dirty) {
  std::lock_guard<std::mutex> lg{latch_};
  auto it_page_table = page_table_.find(page_id);
  if (it_page_table == page_table_.end()) {
    return false;
  }

  auto frame_id = it_page_table->second;
  auto page = &(pages_[frame_id]);
  if (page->GetPinCount() <= 0) {
    return false;
  }

  // 使用判断而非直接赋值是为了避免覆盖以前的状态
  if (is_dirty) {
    page->is_dirty_ = is_dirty;
  }
  page->pin_count_--;

  if (page->pin_count_ <= 0) {
    replacer_->Unpin(frame_id);
  }
  return true;
}

page_id_t BufferPoolManagerInstance::AllocatePage() {
  const page_id_t next_page_id = next_page_id_;
  next_page_id_ += num_instances_;
  ValidatePageId(next_page_id);
  return next_page_id;
}

void BufferPoolManagerInstance::ValidatePageId(const page_id_t page_id) const {
  assert(page_id % num_instances_ == instance_index_);  // allocated pages mod back to this BPI
}

}  // namespace bustub
