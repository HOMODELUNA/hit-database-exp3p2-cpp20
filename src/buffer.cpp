/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"
#include <stdexcept>
namespace badgerdb { 

BufMgr::BufMgr(std::uint32_t bufs)
	: numBufs(bufs),frame_of_each_file_and_page(((((int) (bufs * 1.2))*2)/2)+1){
	frames.reserve(bufs);
  for (FrameId i = 0; i < bufs; i++){
		frames.push_back(StatedPage(i));
  }
  clockHand = bufs - 1;
}


BufMgr::~BufMgr() {
}

void BufMgr::advanceClock(){

}

FrameId BufMgr::allocFrame() {
	
}

	
void BufMgr::readPage(File* file, const PageId pageNo, Page*& page){
	
}


void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty){

}

void BufMgr::flushFile(const File* file){

}

void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page) {

}

void BufMgr::disposePage(File* file, const PageId pageNo){

}

void BufMgr::printSelf(void) 
{
	int validFrames = 0;
  for(auto [i,buf] : enumerate(frames)){
		std::cout << "FrameNo:" << i << " ";
		buf.Print();
		if (buf.valid == true)
    	validFrames++;
	}
  // for (std::uint32_t i = 0; i < numBufs; i++)
	// {
  // 	tmpbuf = &(buffer_descriptors[i]);
	// 	std::cout << "FrameNo:" << i << " ";
	// 	tmpbuf->Print();

  // 	if (tmpbuf->valid == true)
  //   	validFrames++;
  // }

	std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
}

}
