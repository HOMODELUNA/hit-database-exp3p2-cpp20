/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include "hash_table_exception.h"

#include <sstream>
#include <string>

namespace badgerdb {

HashTableException::HashTableException()
    : BadgerDbException(""){
  std::stringstream ss;
  ss << "散列表的缓冲区出现故障.";
  message_.assign(ss.str());
}

}
