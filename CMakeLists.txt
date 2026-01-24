cmake_minimum_required(VERSION 3.16)
project(QtCsv VERSION 1.0.0 LANGUAGES CXX)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core)

# 设置编译选项
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 定义导出宏
if(WIN32)
    add_definitions(-DQTCSV_EXPORTS)
endif()

# 添加头文件列表
set(HEADER_FILES
    include/QCsv.hpp
)

# 添加源文件列表
set(SOURCE_FILES
    src/QCsv.cpp
)

# 添加 QtCsv 库
add_library(QtCsv SHARED
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# 设置版本信息
set_target_properties(QtCsv PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "QtCsv"
    PUBLIC_HEADER "${HEADER_FILES}"
)

# 包含头文件
target_include_directories(QtCsv PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 链接 Qt 模块
target_link_libraries(QtCsv PRIVATE
    Qt6::Core
)

# 可选：如果支持 GUI 功能
option(QTCSV_WIDGETS "Build QtCsv with widgets support" OFF)
if(QTCSV_WIDGETS)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    target_link_libraries(QtCsv PRIVATE Qt6::Widgets)
    
    # 添加 GUI 相关文件
    list(APPEND HEADER_FILES
        include/QtCsv/CsvTableModel.h
        include/QtCsv/CsvTableView.h
    )
    list(APPEND SOURCE_FILES
        src/CsvTableModel.cpp
        src/CsvTableView.cpp
    )
endif()

option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    
    # 查找 Qt6 Test 模块
    find_package(Qt6 REQUIRED COMPONENTS Test)
    
    # 添加测试可执行文件
    add_executable(qtcsv_test
        tests/test.cpp
    )
    
    target_link_libraries(qtcsv_test PRIVATE
        Qt6::Core
        Qt6::Test
        QtCsv
    )
    
    add_test(NAME QtCsvTests COMMAND qtcsv_test)
endif()

# 安装目标
install(TARGETS QtCsv
    EXPORT QtCsvTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION include/QtCsv
)

# 安装头文件目录
install(DIRECTORY include/QtCsv DESTINATION include)

# 生成包配置文件
include(CMakePackageConfigHelpers)

# 创建配置文件
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/QtCsvConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/QtCsvConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtCsv
)

# 创建版本文件
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/QtCsvConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 安装导出文件
install(EXPORT QtCsvTargets
    FILE QtCsvTargets.cmake
    NAMESPACE QtCsv::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtCsv
)

# 安装配置文件
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/QtCsvConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/QtCsvConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtCsv
)

# 添加示例
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()


# 可选：安装 CMake 查找模块
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/FindQtCsv.cmake
    DESTINATION ${CMAKE_INSTALL_DATAROOT}/cmake/Modules
)

# 可选：添加 uninstall 目标
# configure_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
#     IMMEDIATE @ONLY
# )

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)