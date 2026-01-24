# qtcsv

A CSV model for Qt

---

# qtcsv

一个用于 Qt 的 CSV 模型库

## 简介 | Introduction

qtcsv 是一个基于 Qt 框架开发的轻量级 CSV 文件读写与管理库，支持以 Excel 风格的键（如 "A1"）进行数据访问。

qtcsv is a lightweight CSV file read/write and management library developed with the Qt framework. It supports data access using Excel-style keys (e.g., "A1").

## 特性 | Features

- 支持以 Excel 单元格风格的键（如 "A1"）访问数据
- 读写 CSV 文件，支持原子保存
- 可自定义分隔符
- 提供数据搜索与同步功能
- 适用于 Qt 项目，易于集成

- Access data with Excel-style keys (e.g., "A1")
- Read and write CSV files, with atomic save support
- Customizable separator
- Data search and sync functions
- Easy integration for Qt projects

## 安装 | Installation

1. 将 `include/QCsv.hpp` 和 `src/QCsv.cpp` 添加到你的 Qt 项目中。
2. 在你的 `.pro` 或 CMake 配置文件中包含相应路径。

1. Add `include/QCsv.hpp` and `src/QCsv.cpp` to your Qt project.
2. Include the relevant paths in your `.pro` or CMake configuration file.

## 使用示例 | Usage Example

```cpp
#include "QCsv.hpp"

QCsv csv("data.csv");
csv.open("data.csv");
csv.setValue("A1", "Hello");
csv.save();
csv.close();
```

## API 说明 | API Reference

- `QCsv(QString filePath, QObject* parent = nullptr)`  构造函数 | Constructor
- `void open(QString filePath)`  打开文件 | Open file
- `void close()`  关闭文件 | Close file
- `bool isOpen() const`  文件是否已打开 | Is file open
- `void load()`  加载数据 | Load data
- `void save()`  保存数据 | Save data
- `void saveAs(QString filePath)`  另存为 | Save as
- `void clear()`  清空数据 | Clear data
- `void sync()`  同步数据 | Sync data
- `void atomicSave()`  原子保存 | Atomic save
- `void atomicSaveAs(QString filePath)`  原子另存为 | Atomic save as
- `void finalize()`  保存并关闭 | Save and close
- `QString getValue(const QString& key) const`  获取值 | Get value
- `QList<QString> search(QString index) const`  搜索 | Search
- `void setValue(const QString& key, const QString& value)`  设置值 | Set value
- `void setSeparator(char sep)`  设置分隔符 | Set separator
- `char getSeparator() const`  获取分隔符 | Get separator

## 许可证 | License

GPLv3.0 License

---

如需更多信息，请参阅源代码或提交 issue。
For more information, please refer to the source code or submit an issue.
