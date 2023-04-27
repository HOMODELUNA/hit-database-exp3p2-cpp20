/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#pragma once

#include <fstream>
#include <string>
#include <set>
#include <memory>

#include "page.h"

namespace badgerdb {

class FileIterator;

/**
 * @brief 保存了文件中关于页使用情况的信息
 */
struct FileHeader {
  ///
  ///文件中的页数
  PageId num_pages;
  ///
  ///文件中第一个被使用的页数
  PageId first_used_page;
  ///
  ///文件中可用(被分配但未使用)的页数
  PageId num_free_pages;
  ///
  /// 第一个空余(被分配但未使用)的页数
  PageId first_free_page;

  bool operator==(const FileHeader& rhs) const  = default;
};

/**
 * @brief Class which represents a file in the filesystem containing database
 *        pages.
 *
 * The File class wraps a stream to an underlying file on disk.  Files contain
 * fixed-sized pages, and they never deallocate space (though they do reuse
 * deleted pages if possible).  If multiple File objects refer to the same
 * underlying file, they will share the stream in memory.
 * If a file that has already been opened (possibly by another query), then the File class
 * detects this (by looking in the open_streams_ map) and just returns a file object with
 * the already created stream for the file without actually opening the UNIX file again. 
 *
 * @warning This class is not threadsafe.
 */
class File :public std::enable_shared_from_this<File> {
 public:
 using sptr = std::shared_ptr<File>;
  /**
   * 创建一个新文档
   *
   * @throws  FileExistsException     如果文件已经存在
   */
  static sptr create(const std::string& filename);

  /**
   * Opens the file named fileName and returns the corresponding File object.
	 * It first checks if the file is already open. If so, then the new File object created uses the same input-output stream to read to or write fom
	 * that already open file. Reference count (open_counts_ static variable inside the File object) is incremented whenever an already open file is
	 * opened again. Otherwise the UNIX file is actually opened. The fileName and the stream associated with this File object are inserted into the
	 * open_streams_ map.
   *
   * @throws  FileNotFoundException   If the requested file doesn't exist.
   */
  static sptr open(const std::string& filename);

  /**
   * 删除一个已存在的文件.
   *
   * @param filename  Name of the file.
   * @throws  FileNotFoundException   如果文件不存在
   * @throws  FileOpenException       如果文件已经打开
   */
  static void remove(const std::string& filename);

  /**
   * 检查文件是否存在且已经打开
   */
  static bool isOpen(const std::string& filename);


  /**
   * 检查文件是否存在
   *
   * @param filename  Name of the file.
   */
  static bool exists(const std::string& filename);

  

  /**
   * Assignment operator.
   *
   * @param rhs File object to assign.
   * @return    Newly assigned file object.
   */
  File& operator=(const File& rhs) = delete;


  ~File(){opened_files.erase(filename_);stream_.close();}

  /**
   * Allocates a new page in the file.
   *
   * @return The new page.
   */
  Page allocatePage();

  /**
   * Reads an existing page from the file.
   *
   * @param page_number   Number of page to read.
   * @return  The page.
   * @throws  InvalidPageException  If the page doesn't exist in the file or is
   *                                not currently used.
   */
  Page readPage(const PageId page_number) ;

  /**
   * Writes a page into the file, replacing any existing contents.  The page
   * must have been already allocated in this file by a call to allocatePage().
   *
   * @see allocatePage()
   * @param new_page  Page to write.
   */
  void writePage(const Page& new_page);

  /**
   * Deletes a page from the file.
   *
   * @param page_number   Number of page to delete.
   */
  void deletePage(const PageId page_number);

  /**
   * Returns the name of the file this object represents.
   *
   * @return Name of file.
   */
  const std::string& filename() const { return filename_; }

  /**
   * Returns an iterator at the first page in the file.
   *
   * @return  Iterator at first page of file.
   */
  FileIterator begin();

  /**
   * Returns an iterator representing the page after the last page in the file.
   * This iterator should not be dereferenced.
   *
   * @return  Iterator representing page after the last page in the file.
   */
  FileIterator end();
  File(const File& other) = delete;
 private:

  
  /**
   * Returns the position of the page with the given number in the file (as an
   * offset from the beginning of the file).
   *
   * @param page_number   Number of page.
   * @return  Position of page in file.
   */
  static std::streampos pagePosition(const PageId page_number) {
    return sizeof(FileHeader) + ((page_number - 1) * Page::SIZE);
  }

  /**
   * Constructs a file object representing a file on the filesystem.
   * This method should not be called directly; instead use the static methods
   * on this class.
   *
   * @see File::create()
   * @see File::open()
   * @param name        Name of file.
   * @param create_new  Whether to create a new file.
   * @throws  FileExistsException     If the underlying file exists and
   *                                  create_new is true.
   * @throws  FileNotFoundException   If the underlying file doesn't exist and
   *                                  create_new is false.
   */
  File(const std::string& name, std::fstream fs):filename_(name),stream_(std::move(fs)){}

  /**
   * Opens the underlying file named in filename_.
   * This method only opens the file if no other File objects exist that access
   * the same filesystem file; otherwise, it reuses the existing stream.
   *
   * @param create_new  Whether to create a new file.
   * @throws  FileExistsException     If the underlying file exists and
   *                                  create_new is true.
   * @throws  FileNotFoundException   If the underlying file doesn't exist and
   *                                  create_new is false.
   */
  void openIfNeeded(const bool create_new);

  /**
   * Closes the underlying file stream in <stream_>.
   * This method only closes the file if no other File objects exist that access
   * the same file.
   */
  void close();

  /**
   * Reads a page from the file.  If <allow_free> is not set, an exception
   * will be thrown if the page read from disk is not currently in use.
   *
   * No bounds checking is performed; the underlying file stream will throw
   * an exception if the page is past the end of the file.
   *
   * @param page_number   Number of page to read.
   * @param allow_free    Whether to allow reading a free (unused) page.
   * @return  The page.
   * @throws  InvalidPageException  If the page is free (unused) and
   *                                allow_free is false.
   */
  Page readPage(const PageId page_number, const bool allow_free);

  /**
   * Writes a page into the file at the given page number.  This does not
   * update ensure that the number in the header equals the position on disk.
   * No bounds checking is performed.
   *
   * @param page_number Number of page whose contents to replace.
   * @param new_page    Page to write.
   */
  void writePage(const PageId page_number, const Page& new_page);

  /**
   * Writes a page into the file at the given page number with the given header.
   * This does not ensure that the number in the header equals the position on
   * disk.  No bounds checking is performed.
   *
   * @param page_number Number of page whose contents to replace.
   * @param header      Header of page to write.
   * @param new_page    Page to write.
   */
  void writePage(const PageId page_number, const PageHeader& header,
                 const Page& new_page);

  /**
   * 读取文件头
   */
  FileHeader readHeader();

  /**
   * 写入文件头
   */
  void writeHeader(const FileHeader& header);

  /**
   * Reads only the header of the given page from disk (not the record data
   * or slot table).  No bounds checking is performed.
   *
   * @param page_number   Number of page whose header is to be read.
   * @return  Header of page.
   */
  PageHeader readPageHeader(const PageId page_number) ;

  using CountMap =  std::set<std::string>;

  /**
   * Counts for opened files.
   */
  static CountMap opened_files;

  /**
   * Name of the file this object represents.
   */
  std::string filename_;

  /**
   * Stream for underlying filesystem object.
   */
  std::fstream stream_;

  friend class FileIterator;
  friend class FileTest;
};

}
