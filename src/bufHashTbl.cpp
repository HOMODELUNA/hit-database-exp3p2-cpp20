/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <cstdint>
#include <iostream>
#include "buffer.h"
#include "bufHashTbl.h"
#include "exceptions/hash_already_present_exception.h"
#include "exceptions/hash_not_found_exception.h"
#include "exceptions/hash_table_exception.h"

namespace badgerdb {

int BufHashTbl::hash(const File::sptr file, const PageId pageNo)
{
  intptr_t tmp; 
  tmp = (intptr_t)file.get();  // cast of pointer to the file object to an integer
  int value = (tmp + pageNo) % HTSIZE;
  return value;
}

BufHashTbl::BufHashTbl(int htSize)
	: HTSIZE(htSize)
{
  // allocate an array of pointers to hashBuckets
  ht = new hashBucket* [htSize];
  for(int i=0; i < HTSIZE; i++)
    ht[i] = NULL;
}

BufHashTbl::~BufHashTbl()
{
  for(int i = 0; i < HTSIZE; i++) {
    hashBucket* tmpBuf = ht[i];
    while (ht[i]) {
      tmpBuf = ht[i];
      ht[i] = ht[i]->next;
      delete tmpBuf;
    }
  }
  delete [] ht;
}

void BufHashTbl::insert(const File::sptr file, const PageId pageNo, const FrameId frameNo)
{
  int index = hash(file, pageNo);

  hashBucket* tmpBuc = ht[index];
  while (tmpBuc) {
    if (tmpBuc->file == file.get() && tmpBuc->pageNo == pageNo)
  		throw HashAlreadyPresentException(tmpBuc->file->filename(), tmpBuc->pageNo, tmpBuc->frameNo);
    tmpBuc = tmpBuc->next;
  }

  tmpBuc = new hashBucket;
  if (!tmpBuc)
  	throw HashTableException();

  tmpBuc->file = file.get();
  tmpBuc->pageNo = pageNo;
  tmpBuc->frameNo = frameNo;
  tmpBuc->next = ht[index];
  ht[index] = tmpBuc;
}

void BufHashTbl::lookup(const File::sptr file, const PageId pageNo, FrameId &frameNo) 
{
  int index = hash(file, pageNo);
  hashBucket* tmpBuc = ht[index];
  while (tmpBuc) {
    if (tmpBuc->file == file.get() && tmpBuc->pageNo == pageNo)
    {
      frameNo = tmpBuc->frameNo; // return frameNo by reference
      return;
    }
    tmpBuc = tmpBuc->next;
  }

  throw HashNotFoundException(file->filename(), pageNo);
}

void BufHashTbl::remove(const File::sptr file, const PageId pageNo) {

  int index = hash(file, pageNo);
  hashBucket* tmpBuc = ht[index];
  hashBucket* prevBuc = NULL;

  while (tmpBuc)
	{
    if (tmpBuc->file == file.get() && tmpBuc->pageNo == pageNo)
		{
      if(prevBuc) 
				prevBuc->next = tmpBuc->next;
      else
				ht[index] = tmpBuc->next;

      delete tmpBuc;
      return;
    }
		else
		{
      prevBuc = tmpBuc;
      tmpBuc = tmpBuc->next;
    }
  }

  throw HashNotFoundException(file->filename(), pageNo);
}

}
