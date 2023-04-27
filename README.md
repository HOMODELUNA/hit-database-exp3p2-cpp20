# BadgerDB 20                                                  #

HIT 数据库实验3proj2,但是用c++20重构
## 动机
原来的程序太恶心了,并且使用十年前的语法,这个在写的时候很容易写错.所以重构一回

颜值就是战斗力.

## 用法

### 编译
需要 xmake,以及一个支持c++20的编译器

```bash
xmake build
xmake run
```

### 文档
文档已被翻新为中文版
```
xmake doxygen -o docs
```

或
```
doxygen Doxyfile
```

## 你要更改的是:
- `buffer.cpp`: 各种方法的空都留在这里了,你需要写出实现.
