/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include "hash_already_present_exception.h"

#include <sstream>
#include <string>

namespace badgerdb {

HashAlreadyPresentException::HashAlreadyPresentException(const std::string& nameIn, PageId pageNoIn, FrameId frameNoIn)
    : BadgerDbException(""), name(nameIn), pageNo(pageNoIn), frameNo(frameNoIn) {
  std::stringstream ss;
  ss << "文件 " << name << " 的第 " << pageNo << "页在散列表中已经有项.";
  message_.assign(ss.str());
}

}
