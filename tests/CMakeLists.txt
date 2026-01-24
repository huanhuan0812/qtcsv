# test/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(QtCsvTest)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core)

# 设置编译选项
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 包含 QCsv 头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加测试可执行文件
add_executable(qtcsv_test
    test.cpp
)

# 链接库
target_link_libraries(qtcsv_test
    Qt6::Core
    QtCsv  # 链接到你的 QtCsv 库
)

# 设置输出目录
set_target_properties(qtcsv_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)