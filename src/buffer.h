/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#pragma once

#include "file.h"
#include "bufHashTbl.h"
#include <iostream>
#include<vector>
#include<memory>
namespace badgerdb {

/**
* forward declaration of BufMgr class 
*/
class BufMgr;
class PageView;class MutablePageView;
/**
* @brief 包含一个真正的页,并记录了它的信息
*/
class StatedPage {
	friend class BufMgr;friend class PageView;friend class MutablePageView;
 private:
	//Pointer to file to which corresponding frame is assigned
  File::sptr file;
  //Page within file to which corresponding frame is assigned
  PageId pageNo;
  //Frame number of the frame, in the buffer pool, being used
  const FrameId	frameNo;
  //Number of times this page has been pinned
  int pinCnt;
  ///这个页是否是脏的
  bool dirty;
  //这个页是否可用(对外部使用者来说)
  bool valid;
  //这个页是否最近被引用
  bool recently_referenced;

	// 真正的页
	Page data;
	/**
   * 为新用户初始化缓冲区
	 */
  void Clear()	{
    pinCnt = 0;
		file = NULL;
		pageNo = Page::INVALID_NUMBER;
    dirty = false;
    recently_referenced = false;
		valid = false;
  };
	//空闲----valid 的反义词
	bool empty()const{return !valid;}
	//向文件中写入该页
	void dump_to_file(){
		if(file == nullptr){throw std::invalid_argument("file is empty: "+file->filename());}
		//std::cerr<<"[debug] dumping "<<file->filename()<<" page "<<pageNo<<"\n";
		file->writePage(data);
	}
	/**
	 * 将成员变量设为它所映射的文件中,和文件中的页号.
	 * 在 readPage() or allocPage() 中,当一个空页被绑定到一个现存的页时调用. 
	 *
	 * @param filePtr	文件
	 * @param pageNum	文件中的页号
	 */
  void occupy_for(File::sptr filePtr, PageId pageNum)	{ 
		if(filePtr == nullptr){throw std::invalid_argument("file is empty: "+file->filename());}
		file = filePtr;
    pageNo = pageNum;
    pinCnt = 1;
    dirty = false;
    valid = true;
    recently_referenced = true;
  }

  void Print()const	{
		if(file){
			std::cout << "file:" << file->filename() << " ";
			std::cout << "pageNo:" << pageNo << " ";
		}
		else
			std::cout << "file:NULL ";

		std::cout << "valid:" << valid << " ";
		std::cout << "pinCnt:" << pinCnt << " ";
		std::cout << "dirty:" << dirty << " ";
		std::cout << "recently_referenced:" << recently_referenced << "\n";
  }
	public:
	/**
   * Constructor of BufDesc class 
	 */
  StatedPage(FrameId frame_id):frameNo(frame_id)	{  	Clear();  }
};


/// @brief 不可写,但是 \b 易变 的页的视图. 
/// 它只保证自己不会写这个页面,但是不会保证自己读相同的项会得出相同的结果
/// (因为可能有别的可变视图在写这个页面)
/// 
/// 当自己析构时,会向管理器归还页面(并表示自己没有写这个页面)
class PageView{
	const StatedPage* stpage;
	const Page * page;
	BufMgr & mgr;
	public:
	PageView(const PageView&) = delete;
	PageView& operator=(PageView&&) = default;
	PageView(PageView&& b):mgr(b.mgr){page =  b.page ;stpage = b.stpage; b.page = nullptr;}
	PageView(const StatedPage* _stpage,BufMgr& _mgr):mgr(_mgr),stpage(_stpage){
		if(stpage == nullptr){throw std::invalid_argument("stpage 为空指针");}
		page = & _stpage ->data;
	}
	const Page* operator->()const{return page;}
	/// @brief 放弃自己对页的引用,这样它们就不再需要维持在内存中了.
	///
	void unpin(){
		if(page){
			mgr.unPinPage(stpage->file,stpage->pageNo,false);
		}
		page = nullptr;
	}
	~PageView(){unpin();}
};


/// @brief 可写,且 \b 易变 的页的视图.
/// 它不能保证自己对页面的修改一定会被保留 
/// (因为可能有别的可变视图在写这个页面,且恰好写了相同的位置)
/// 
/// 当自己析构时,会向管理器归还页面(并表示自己 \b 写了 这个页面)
class MutablePageView {
	StatedPage* stpage;
	Page * page;
	BufMgr & mgr;
	public:
	MutablePageView(const MutablePageView&) = delete;
	MutablePageView& operator=(MutablePageView&&) = default;
	MutablePageView(MutablePageView&& b):mgr(b.mgr){page =  b.page ;stpage = b.stpage; b.page = nullptr;}
	MutablePageView(StatedPage* _stpage,BufMgr& _mgr):mgr(_mgr),stpage(_stpage){
		if(stpage == nullptr){throw std::invalid_argument("stpage 为空指针");}
		page =&  _stpage->data;
	}
	Page* operator->(){return page;}
	/// @brief 生成一个不改变的视图
	/// 
	PageView to_immut()const{return PageView(stpage,mgr);}
	/// @brief 放弃自己对页的引用,这样它们就不再需要维持在内存中了.
	///
	void unpin(){
		if(page){
			mgr.unPinPage(stpage->file,stpage->pageNo,true);
		}
		page = nullptr;
	}
	~MutablePageView(){unpin();}
};


template<typename T>
concept is_page_view = std::same_as<T,PageView> || std::same_as<T,MutablePageView>;

/**
* @brief 缓冲区使用情况的统计信息
*/
struct BufStats{
  //缓冲池的总访问次数
  int accesses;
  //Number of pages read from disk (including allocs)
  int diskreads;
  //Number of pages written back to disk
  int diskwrites;
  //Clear all values to zero
  void clear()  {		accesses = diskreads = diskwrites = 0;  }
  BufStats() {		clear();  }
};


/**
* @brief The central class which manages the buffer pool including frame allocation and deallocation to pages in the file 
*/
class BufMgr {
	friend class PageView; friend class MutablePageView;
 private:
  //Current position of clockhand in our buffer pool
  FrameId clockHand;
  //Number of frames in the buffer pool
  const std::uint32_t numBufs;	
  //Hash table mapping (File, page) to frame
  BufHashTbl frame_of_each_file_and_page;
  //Array of BufDesc objects to hold information corresponding to every frame allocation from 'bufPool' (the buffer pool)
  std::vector<StatedPage> frames;
  //Maintains Buffer pool usage statistics 
  BufStats bufStats;

	/**
   * Advance clock to next frame in the buffer pool
	 */
  void advanceClock();

	/**
	 * 分配一个空闲的帧.  
	 *
	 * @throws BufferExceededException 如果找不到一个可用的帧
	 */
  FrameId allocFrame();
	/**
	 * @brief 找到一个内部的页,供PageView 包装
	 * 
	 * @param file 
	 * @param PageNo 
	 * @return StatedPage& 返回的内部页
	 */
	StatedPage& readPageInner(File::sptr file, const PageId PageNo);
 public:
  
  BufMgr(std::uint32_t bufs);
  ~BufMgr();

	/**
	 * Reads the given page from the file into a frame and returns the pointer to page.
	 * If the requested page is already present in the buffer pool pointer to that frame is returned
	 * otherwise a new frame is allocated from the buffer pool for reading the page.
	 *
	 * @param file   	File object
	 * @param PageNo  Page number in the file to be read
	 * @param page  	Reference to page pointer. Used to fetch the Page object in which requested page from file is read in.
	 */
	template<typename IPageView = PageView>
  IPageView readPage(File::sptr file, const PageId PageNo){
		return IPageView(readPageInner(file,PageNo),*this);
	}

	

	PageView readPageAsMutable(File::sptr file, const PageId PageNo, Page*& page);
	/**
	 * Allocates a new, empty page in the file and returns the Page object.
	 * The newly allocated page is also assigned a frame in the buffer pool.
	 *
	 * @param file   	File object
	 * @param PageNo  Page number. The number assigned to the page in the file is returned via this reference.
	 * @param page  	Reference to page pointer. The newly allocated in-memory Page object is returned via this reference.
	 */
  void allocPage(File::sptr file, PageId &PageNo, Page*& page); 


	/**
	 * 减少页的引用,因为它们已经不再需要维持在内存中了.
	 *
	 * @param file   	文件对象
	 * @param PageNo  页号
	 * @param dirty		这个被取消的页是否需要为脏
   * @throws  PageNotPinnedException 如果页面没有被引用
	 * @warning PageView 在析构时会自动地调用这个函数,请勿手动调用它,以免造成重复释放
	 */
  void unPinPage(File::sptr file, const PageId PageNo, const bool dirty);

	/**
	 * Writes out all dirty pages of the file to disk.
	 * All the frames assigned to the file need to be unpinned from buffer pool before this function can be successfully called.
	 * Otherwise Error returned.
	 *
	 * @param file   	File object
   * @throws  PagePinnedException If any page of the file is pinned in the buffer pool 
   * @throws BadBufferException If any frame allocated to the file is found to be invalid
	 */
  void flushFile(File::sptr file);

	/**
	 * Delete page from file and also from buffer pool if present.
	 * Since the page is entirely deleted from file, its unnecessary to see if the page is dirty.
	 *
	 * @param file   	File object
	 * @param PageNo  Page number
	 */
  void disposePage(File::sptr file, const PageId PageNo);

  //Print member variable values. 
  void  printSelf();

  //Get buffer pool usage statistics
  BufStats & getBufStats(){		return bufStats;  }
  //Clear buffer pool usage statistics
  void clearBufStats()   {		bufStats.clear();  }
};

}
