# BadgerDB quick start guide                                                   #

### 你要更改的是:
`buffer.cpp`: Skeleton implementation of the methods. Provide your actual implementation here

用法
```
xmake build
xmake run
```
# Building the source and documentation                                        #

To build the source:
  $ make

To build the real API documentation (requires Doxygen):
  $ make doc

To view the documentation, open docs/index.html in your web browser after
running make doc.

# Prerequisites                                                                #

If you are running this on a CSL instructional machine, these are taken care of.

Otherwise, you need:
 * a modern C++ compiler (gcc version 4.6 or higher, any recent version of clang)
 * doxygen (version 1.4 or higher)
