
# QtCsv

## English

QtCsv is a simple and easy-to-use library for reading and writing CSV files with Qt. It provides convenient APIs to handle CSV data in Qt applications.

**Note:** Currently, Chinese language is not supported in CSV reading/writing.

### Features
- Read and write CSV files easily
- Integrate with Qt data structures
- Lightweight and header-only

### Usage
1. Add `QCsv.hpp` and `QCsv.cpp` to your project.
2. Include the header in your code:
	```cpp
	#include "QCsv.hpp"
	```
3. Use the provided classes to read or write CSV files.

### Build
Use CMake to build the project:
```sh
mkdir build && cd build
cmake ..
make
```

### License
See [LICENSE](LICENSE) for details.

---

## 中文说明

QtCsv 是一个基于 Qt 的简单易用的 CSV 文件读写库，适用于需要在 Qt 应用中处理 CSV 数据的场景。

**注意：目前暂不支持中文内容的读写。**

### 特性
- 简单读写 CSV 文件
- 与 Qt 数据结构集成
- 轻量级，头文件实现

### 用法
1. 将 `QCsv.hpp` 和 `QCsv.cpp` 添加到你的项目中。
2. 在代码中包含头文件：
	```cpp
	#include "QCsv.hpp"
	```
3. 使用库中提供的类进行 CSV 文件的读写。

### 构建
使用 CMake 构建项目：
```sh
mkdir build && cd build
cmake ..
make
```

### 许可证
详情见 [LICENSE](LICENSE)。
