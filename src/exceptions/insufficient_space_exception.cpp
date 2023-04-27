/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include "insufficient_space_exception.h"

#include <sstream>
#include <string>

namespace badgerdb {

InsufficientSpaceException::InsufficientSpaceException(
    const PageId page_num, const std::size_t requested,
    const std::size_t available)
    : BadgerDbException(""),
      page_number_(page_num),
      space_requested_(requested),
      space_available_(available) {
  std::stringstream ss;
  ss << "页 " << page_number_
     << " 的空间不足以容纳记录. 需要: " << space_requested_ << " 字节."
     << " 可用: " << space_available_ << " 字节.";
  message_.assign(ss.str());
}

}
