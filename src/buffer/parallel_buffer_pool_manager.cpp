//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// parallel_buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/parallel_buffer_pool_manager.h"

namespace bustub {

ParallelBufferPoolManager::ParallelBufferPoolManager(size_t num_instances, size_t pool_size, DiskManager *disk_manager,
                                                     LogManager *log_manager) {
  // Allocate and create individual BufferPoolManagerInstances
  for (size_t i = 0; i < num_instances; ++i) {
    m_instances.push_back(
        std::make_unique<BufferPoolManagerInstance>(pool_size, num_instances, i, disk_manager, log_manager));
  }
}

// Update constructor to destruct all BufferPoolManagerInstances and deallocate any associated memory
ParallelBufferPoolManager::~ParallelBufferPoolManager() = default;

size_t ParallelBufferPoolManager::GetPoolSize() {
  // Get size of all BufferPoolManagerInstances
  return m_instances.size() * m_instances[0]->GetPoolSize();
}

BufferPoolManager *ParallelBufferPoolManager::GetBufferPoolManager(page_id_t page_id) {
  // Get BufferPoolManager responsible for handling given page id. You can use this method in your other methods.
  auto index = page_id % m_instances.size();
  return m_instances[index].get();
}

Page *ParallelBufferPoolManager::FetchPgImp(page_id_t page_id) {
  // Fetch page for page_id from responsible BufferPoolManagerInstance
  auto *instance = GetBufferPoolManager(page_id);
  return instance->FetchPage(page_id);
}

bool ParallelBufferPoolManager::UnpinPgImp(page_id_t page_id, bool is_dirty) {
  // Unpin page_id from responsible BufferPoolManagerInstance
  auto *instance = GetBufferPoolManager(page_id);
  return instance->UnpinPage(page_id, is_dirty);
}

bool ParallelBufferPoolManager::FlushPgImp(page_id_t page_id) {
  // Flush page_id from responsible BufferPoolManagerInstance
  auto *instance = GetBufferPoolManager(page_id);
  return instance->FlushPage(page_id);
}

Page *ParallelBufferPoolManager::NewPgImp(page_id_t *page_id) {
  // create new page. We will request page allocation in a round robin manner from the underlying
  // BufferPoolManagerInstances
  // 1.   From a starting index of the BPMIs, call NewPageImpl until either 1) success and return 2) looped around to
  // starting index and return nullptr
  // 2.   Bump the starting index (mod number of instances) to start search at a different BPMI each time this function
  // is called
  std::lock_guard<std::mutex> lg{m_latch};
  for (size_t index = m_start_index; index < m_start_index + m_instances.size(); ++index) {
    size_t mod_index = index % m_instances.size();
    auto *page = m_instances[mod_index]->NewPage(page_id);
    if (page != nullptr) {
      m_start_index = (m_start_index + 1) % m_instances.size();
      return page;
    }
  }
  m_start_index = (m_start_index + 1) % m_instances.size();
  return nullptr;
}

bool ParallelBufferPoolManager::DeletePgImp(page_id_t page_id) {
  auto *instance = GetBufferPoolManager(page_id);
  return instance->DeletePage(page_id);
}

void ParallelBufferPoolManager::FlushAllPgsImp() {
  // flush all pages from all BufferPoolManagerInstances
  for (auto &ins : m_instances) {
    ins->FlushAllPages();
  }
}

}  // namespace bustub
