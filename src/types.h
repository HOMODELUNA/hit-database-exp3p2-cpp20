/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#pragma once
#include<cstdint>
namespace badgerdb {

/**
 * @brief 文件中的页号
 */
using  PageId = uint32_t;

/**
 * @brief 页中的插槽号.
 */
using SlotId = uint16_t;

/**
 * @brief 缓冲池中的帧号.
 */
using FrameId = uint32_t;

/**
 * @brief 页中记录项的标识符.
 */
struct RecordId {
  /**
   * 保有此项的页号.
   */
  PageId page_number;

  /**
   * 页中的插槽号.
   */
  SlotId slot_number;
  bool operator==(const RecordId& rhs) const = default;
};

}
