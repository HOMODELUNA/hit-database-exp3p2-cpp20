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
#include "iter/round.hpp"
#include "iter/enumerate.hpp"
namespace badgerdb { 

using Iter::enumerate;
BufMgr::BufMgr(std::uint32_t bufs)
	: numBufs(bufs),frames(bufs),bufPool(bufs),frame_of_each_file_and_page(((((int) (bufs * 1.2))*2)/2)+1){

  for (FrameId i = 0; i < bufs; i++) 
  {
  	frames[i].frameNo = i;
  	frames[i].valid = false;
  }
  clockHand = bufs - 1;
}


BufMgr::~BufMgr() {
}

void BufMgr::advanceClock(){
	clockHand = (clockHand+1) % numBufs;
}

FrameId BufMgr::allocFrame() {
	bool have_a_potential_available_block=false;
	for(int i =0; i< numBufs || have_a_potential_available_block;++i){
		advanceClock();
		auto& f = frames.at(clockHand);
		if(f.empty()){
			return clockHand;
		}
		if(f.pinCnt !=0){ continue;	}
		have_a_potential_available_block = true;
		if(f.refbit){
			f.refbit = false;
			continue;
		}
		if(f.dirty){
			f.dump_to_file();
			f.dirty = false;
		}
		frame_of_each_file_and_page.remove(f.file,f.pageNo);
		f.Clear();
		return clockHand;
	}
	throw BufferExceededException();
}

	
void BufMgr::readPage(File* file, const PageId pageNo, Page*& page){
	if(file == nullptr){throw std::invalid_argument("file is empty: "+file->filename());}
	try{
		FrameId fid;
		frame_of_each_file_and_page.lookup(file,pageNo,fid);
		auto & f = frames[fid];
		f.refbit = true;
		++ f.pinCnt;
		//std::cerr<<"[debug] frame file "<<file->filename()<<" page "<<f.pageNo<<" pin count now is "<<f.pinCnt<<"\n";
		page = & f.data;
		return;
	}
	catch(const HashNotFoundException& e){
		FrameId fid = allocFrame();
		auto & f = frames[fid];
		f.data = file ->readPage(pageNo);
		frame_of_each_file_and_page.insert(file,pageNo,fid);
		f.Set(file,pageNo);
		page = & f.data;
		return; 
	}
	
}


void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty){
	if(file == nullptr){throw std::invalid_argument("file is empty: "+file->filename());}
	FrameId fid = -1;
	frame_of_each_file_and_page.lookup(file,pageNo,fid);
	StatedPage & f = frames[fid];
	if (f.pinCnt <= 0){
		throw PageNotPinnedException(file->filename(),pageNo,fid);
	}
	f.pinCnt -=1;
	//std::cerr<<"[debug] frame file "<<file->filename()<<" page "<<f.pageNo<<" pin count now is "<<f.pinCnt<<"\n";
	if(dirty){
		f.dirty = true; 
	}
}

void BufMgr::flushFile(const File* file){
	if(file == nullptr){throw std::invalid_argument("file is empty: "+file->filename());}
	for(auto&f: frames){
		if(f.file != file){ continue;	}
		if((! f.valid) || f.pinCnt!=0){
			throw PagePinnedException(f.file->filename(),f.pageNo,f.frameNo);
		}
		if(! f.dirty){continue;}
		f.dump_to_file();
		f.dirty = false;
	}
}

void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page) {
	if(file == nullptr){throw std::invalid_argument("file is empty: "+file->filename());}
	auto fid = allocFrame();
	auto & f =frames[fid];
	f.data = file->allocatePage();
	//std::cerr<<"[debug] new f has id "<<fid<<", pagenum "<< f.data.page_number()<<"\n";
	f.Set(file,f.data.page_number());
	frame_of_each_file_and_page.insert(file,f.pageNo,fid);
	pageNo = f.data.page_number();
	page = & f.data;
}

void BufMgr::disposePage(File* file, const PageId pageNo){
	if(file == nullptr){throw std::invalid_argument("file is empty: "+file->filename());}
	FrameId fid;
	frame_of_each_file_and_page.lookup(file,pageNo,fid);
	frame_of_each_file_and_page.remove(file,pageNo);
	frames[fid].Clear();
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
