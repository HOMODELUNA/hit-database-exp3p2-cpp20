/**
 * @mainpage BadgerDB 文档
 *
 * @section toc_sec 目录
 *
 * <ol>
 *   <li> @ref file_layout_sec
 *   <li> @ref building_sec
 *   <ol>
 *     <li> @ref prereq_sec
 *     <li> @ref commands_sec
 *     <li> @ref modify_run_main_sec
 *     <li> @ref documentation_sec
 *   </ol>
 *   <li> @ref api_sec
 *   <ol>
 *     <li> @ref storage_sec
 *     <ol>
 *       <li> @ref file_management_sec
 *       <li> @ref file_data_sec
 *       <li> @ref page_sec
 *     </ol>
 *   </ol>
 * </ol>
 *
 * @section file_layout_sec 文件布局
 *
 * 包内的文件如下布置:
 * <pre>
 * docs/                  生成的文档
 * src/                   BadgerDB 代码
 * </pre>
 *
 * 你主要操作的是 <code>src</code> 里的文件
 *
 * @section building_sec 构建和更改
 *
 * @subsection prereq_sec 要求
 *
 * 为了编译这个系统,你需要如下软件:
 * <ul>
 *   <li>A modern C++ compiler (需要c++20)
 *   <li>Doxygen 至少 1.6 版本 (用于生成文档,可选)
 * </ul>
 *
 *
 * @subsection commands_sec 构建
 *
 * 制作可执行文件:
 * @code
 *   $ xmake
 * @endcode
 *
 * @subsection modify_run_main_sec 更改和运行 main 函数
 *
 * 构建完成之后,说:
 * @code
 *   $ ./xmake run
 * @endcode
 * 或者:
 * @code
 *   $ ./badgerdb_main
 * @endcode
 * 如果你想定制 <code>badgerdb_main</code>的行为, 编辑
 * <code>src/main.cpp</code>.
 *
 * @subsection documentation_sec 重新构建文档
 *
 * 本文档用 Doxygen 构建.  如果你更新了源码中的文档,而需要重新构建,那么运行:
 * @code
 *  $ doxygen Doxyfile
 * @endcode
 * 见过会被放在 <code>docs/</code>目录下,在浏览器中打开 <code>index.html</code> 以查看.
 *
 * @section api_sec BadgerDB API
 *
 * @subsection storage_sec 文件存储
 *
 * Interaction with the underlying filesystem is handled by two classes: File
 * 和 Page 类用于与下层文件系统交互.  Files 储存零个或多个固定长度的页,每页拥有一些变长记录(record) 
 *
 * Record 的数据是任意长度的 std::string.
 *
 * @subsubsection file_management_sec 创建/打开/删除文件(File)
 *
 * 文件在使用之前必须创建:
 * @code
 *  // 以名字 "filename.db" 创建并打开文档
 *  badgerdb::File new_file = badgerdb::File::create("filename.db");
 * @endcode
 * 
 * 如果你想要打开一个已经存在的文档,如此使用 File::open :
 * @code
 *  // 打开名为 "filename.db" 的文件(要求文件存在)
 *  badgerdb::File existing_file = badgerdb::File::open("filename.db");
 * @endcode
 *
 * Multiple File objects share the same stream to the underlying file.  The
 * stream will be automatically closed when the last File object is out of
 * scope; no explicit close command is necessary.
 *
 * You can delete a file with File::remove:
 * @code
 *  // Delete a file with the name "filename.db".
 *  badgerdb::File::remove("filename.db");
 * @endcode
 *
 * @subsubsection file_data_sec Reading and writing data in a file
 *
 * Data is added to a File by first allocating a Page, populating it with data,
 * and then writing the Page back to the File.
 *
 * For example:
 * @code
 *   #include "file.h"
 *
 *   ...
 *
 *   // Write a record with the value "hello, world!" to the file.
 *   badgerdb::File db_file = badgerdb::File::open("filename.db");
 *   badgerdb::Page new_page = db_file.allocatePage();
 *   new_page.insertRecord("hello, world!");
 *   db_file.writePage(new_page);
 * @endcode
 *
 * Pages are read back from a File using their page numbers:
 * @code
 *   #include "file.h"
 *   #include "page.h"
 *
 *   ...
 *
 *   // Allocate a page and then read it back.
 *   badgerdb::Page new_page = db_file.allocatePage();
 *   db_file.writePage(new_page);
 *   const badgerdb::PageId& page_number = new_page.page_number();
 *   badgerdb::Page same_page = db_file.readPage(page_number);
 * @endcode
 *
 * You can also iterate through all pages in the File:
 * @code
 *   #include "file_iterator.h"
 *
 *   ...
 *
 *   for (badgerdb::FileIterator iter = db_file.begin();
 *        iter != db_file.end();
 *        ++iter) {
 *     std::cout << "Read page: " << iter->page_number() << std::endl;
 *   }
 * @endcode
 *
 * @subsubsection page_sec Reading and writing data in a page
 *
 * Pages hold variable-length records containing arbitrary data.
 *
 * To insert data on a page:
 * @code
 *   #include "page.h"
 *
 *   ...
 *
 *   badgerdb::Page new_page;
 *   new_page.insertRecord("hello, world!");
 * @endcode
 *
 * Data is read by using RecordIds, which are provided when data is inserted:
 * @code
 *   #include "page.h"
 *
 *   ...
 *
 *   badgerdb::Page new_page;
 *   const badgerdb::RecordId& rid = new_page.insertRecord("hello, world!");
 *   new_page.getRecord(rid); // returns "hello, world!"
 * @endcode
 *
 * As Pages use std::string to represent data, it's very natural to insert
 * strings; however, any data can be stored:
 * @code
 *   #include "page.h"
 *
 *   ...
 *
 *   struct Point {
 *     int x;
 *     int y;
 *   };
 *   Point new_point = {10, -5};
 *   badgerdb::Page new_page;
 *   std::string new_data(reinterpret_cast<char*>(&new_point),
 *                        sizeof(new_point));
 *   const badgerdb::RecordId& rid = new_page.insertRecord(new_data);
 *   Point read_point =
 *       *reinterpret_cast<const Point*>(new_page.getRecord(rid).data());
 * @endcode
 * Note that serializing structures like this is not industrial strength; it's
 * better to use something like Google's protocol buffers or Boost
 * serialization.
 *
 * You can also iterate through all records in the Page:
 * @code
 *   #include "page_iterator.h"
 *
 *   ...
 *
 *   for (badgerdb::PageIterator iter = new_page.begin();
 *        iter != new_page.end();
 *        ++iter) {
 *     std::cout << "Record data: " << *iter << std::endl;
 *   }
 * @endcode
 *
 */
