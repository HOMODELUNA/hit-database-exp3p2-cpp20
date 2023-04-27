/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include "buffer_exceeded_exception.h"

#include <sstream>
#include <string>

namespace badgerdb {

BufferExceededException::BufferExceededException()
    : BadgerDbException(""){
  std::stringstream ss;
  ss << "缓冲池容量不足";
  message_.assign(ss.str());
}

}
