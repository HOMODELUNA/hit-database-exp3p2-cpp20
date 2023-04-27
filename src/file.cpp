/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include "file.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <cstdio>
#include <cassert>

#include "exceptions/file_exists_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/file_open_exception.h"
#include "exceptions/invalid_page_exception.h"
#include "file_iterator.h"
#include "page.h"

namespace badgerdb {

constexpr std::ios_base::openmode OPEN_MODE =std::fstream::in | std::fstream::out | std::fstream::binary;

File::CountMap File::opened_files;

File::sptr File::create(const std::string& filename) {
  if(exists(filename)){throw FileExistsException(filename);  }
  auto res = std::make_shared<File>(filename, std::fstream(filename ,OPEN_MODE));

  FileHeader header = {1 /* num_pages */, 0 /* first_used_page */,
                         0 /* num_free_pages */, 0 /* first_free_page */};
  res -> writeHeader(header);
  return res;
}

File::sptr File::open(const std::string& filename) {
  if(! exists(filename)){throw FileNotFoundException(filename);  }
  return std::make_shared<File>(filename, std::fstream(filename ,OPEN_MODE));
}

void File::remove(const std::string& filename) {
  if (!exists(filename)) {
    throw FileNotFoundException(filename);
  }
  if (isOpen(filename)) {
    throw FileOpenException(filename);
  }
  std::remove(filename.c_str());
}

bool File::isOpen(const std::string& filename) {
  if (!exists(filename)) {
    return false;
  }
  return opened_files.contains(filename);
}

bool File::exists(const std::string& filename) {
	std::fstream file(filename);
	if(file)
	{
		file.close();
		return true;
	}

	return false;
}




File::~File() {
  close();
}

Page File::allocatePage() {
  FileHeader header = readHeader();
  Page new_page;
  Page existing_page;
  if (header.num_free_pages > 0) {
    new_page = readPage(header.first_free_page, true /* allow_free */);
    new_page.set_page_number(header.first_free_page);
    header.first_free_page = new_page.next_page_number();
    --header.num_free_pages;

    if (header.first_used_page == Page::INVALID_NUMBER ||
        header.first_used_page > new_page.page_number()) {
      // Either have no pages used or the head of the used list is a page later
      // than the one we just allocated, so add the new page to the head.
      if (header.first_used_page > new_page.page_number()) {
        new_page.set_next_page_number(header.first_used_page);
      }
      header.first_used_page = new_page.page_number();
    } else {
      // New page is reused from somewhere after the beginning, so we need to
      // find where in the used list to insert it.
      PageId next_page_number = Page::INVALID_NUMBER;
      for (FileIterator iter = begin(); iter != end(); ++iter) {
        next_page_number = (*iter).next_page_number();
        if (next_page_number > new_page.page_number() ||
            next_page_number == Page::INVALID_NUMBER) {
          existing_page = *iter;
          break;
        }
      }
      existing_page.set_next_page_number(new_page.page_number());
      new_page.set_next_page_number(next_page_number);
    }

    assert((header.num_free_pages == 0) ==
           (header.first_free_page == Page::INVALID_NUMBER));
  } else {
    new_page.set_page_number(header.num_pages);
    if (header.first_used_page == Page::INVALID_NUMBER) {
      header.first_used_page = new_page.page_number();
    } else {
      // If we have pages allocated, we need to add the new page to the tail
      // of the linked list.
      for (FileIterator iter = begin(); iter != end(); ++iter) {
        if ((*iter).next_page_number() == Page::INVALID_NUMBER) {
          existing_page = *iter;
          break;
        }
      }
      assert(existing_page.isUsed());
      existing_page.set_next_page_number(new_page.page_number());
    }
    ++header.num_pages;
  }
  writePage(new_page.page_number(), new_page);
  if (existing_page.page_number() != Page::INVALID_NUMBER) {
    // If we updated an existing page by inserting the new page into the
    // used list, we need to write it out.
    writePage(existing_page.page_number(), existing_page);
  }
  writeHeader(header);

  return new_page;
}

Page File::readPage(const PageId page_number) {
  FileHeader header = readHeader();
  if (page_number >= header.num_pages) {
    throw InvalidPageException(page_number, filename_);
  }
  return readPage(page_number, false /* allow_free */);
}

Page File::readPage(const PageId page_number, const bool allow_free) {
  Page page;
  stream_.seekg(pagePosition(page_number), std::ios::beg);
  stream_.read(reinterpret_cast<char*>(&page.header_), sizeof(page.header_));
  stream_.read(reinterpret_cast<char*>(&page.data_[0]), Page::DATA_SIZE);
  if (!allow_free && !page.isUsed()) {
    throw InvalidPageException(page_number, filename_);
  }

  return page;
}

void File::writePage(const Page& new_page) {
  PageHeader header = readPageHeader(new_page.page_number());
  if (header.current_page_number == Page::INVALID_NUMBER) {
    // Page has been deleted since it was read.
    throw InvalidPageException(new_page.page_number(), filename_);
  }
  // Page on disk may have had its next page pointer updated since it was read;
  // we don't modify that, but we do keep all the other modifications to the
  // page header.
  const PageId next_page_number = header.next_page_number;
  header = new_page.header_;
  header.next_page_number = next_page_number;
  writePage(new_page.page_number(), header, new_page);
}

void File::deletePage(const PageId page_number) {
  FileHeader header = readHeader();
  Page existing_page = readPage(page_number);
  Page previous_page;
  // If this page is the head of the used list, update the header to point to
  // the next page in line.
  if (page_number == header.first_used_page) {
    header.first_used_page = existing_page.next_page_number();
  } else {
    // Walk the used list so we can update the page that points to this one.
    for (FileIterator iter = begin(); iter != end(); ++iter) {
      previous_page = *iter;
      if (previous_page.next_page_number() == existing_page.page_number()) {
        previous_page.set_next_page_number(existing_page.next_page_number());
        break;
      }
    }
  }
  // Clear the page and add it to the head of the free list.
  existing_page.initialize();
  existing_page.set_next_page_number(header.first_free_page);
  header.first_free_page = page_number;
  ++header.num_free_pages;
  if (previous_page.isUsed()) {
    writePage(previous_page.page_number(), previous_page);
  }
  writePage(page_number, existing_page);
  writeHeader(header);
}

FileIterator File::begin() {
  const FileHeader& header = readHeader();
  return FileIterator(this, header.first_used_page);
}

FileIterator File::end() {
  return FileIterator(this, Page::INVALID_NUMBER);
}



void File::close() {
  stream_.close();
}

void File::writePage(const PageId page_number, const Page& new_page) {
  writePage(page_number, new_page.header_, new_page);
}

void File::writePage(const PageId page_number, const PageHeader& header,
                     const Page& new_page) {
  stream_.seekp(pagePosition(page_number), std::ios::beg);
  stream_.write(reinterpret_cast<const char*>(&header), sizeof(header));
  stream_.write(reinterpret_cast<const char*>(&new_page.data_[0]),
                 Page::DATA_SIZE);
  stream_.flush();
}

FileHeader File::readHeader() {
  FileHeader header;
  stream_.seekg(0 /* pos */, std::ios::beg);
  stream_.read(reinterpret_cast<char*>(&header), sizeof(header));

  return header;
}

void File::writeHeader(const FileHeader& header) {
  stream_.seekp(0 /* pos */, std::ios::beg);
  stream_.write(reinterpret_cast<const char*>(&header), sizeof(header));
  stream_.flush();
}

PageHeader File::readPageHeader(PageId page_number) {
  PageHeader header;
  stream_.seekg(pagePosition(page_number), std::ios::beg);
  stream_.read(reinterpret_cast<char*>(&header), sizeof(header));

  return header;
}

}
