// test.cpp
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <iostream>
#include "include/QCsv.hpp"

// 测试文件路径
const QString TEST_FILE = "test_data.csv";
const QString TEST_FILE_SAVE = "test_data_save.csv";
const QString TEST_FILE_ATOMIC = "test_data_atomic.csv";

// 清理测试文件
void cleanupTestFiles() {
    QFile::remove(TEST_FILE);
    QFile::remove(TEST_FILE_SAVE);
    QFile::remove(TEST_FILE_ATOMIC);
}

// 创建测试数据文件
void createTestCsvFile() {
    QFile file(TEST_FILE);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "A1,B1,C1\n";
        out << "A2,B2,C2\n";
        out << "A3,B3,C3\n";
        out << "Name,Age,City\n";
        out << "John,25,New York\n";
        out << "Alice,30,London\n";
        out << "Bob,35,Tokyo\n";
        file.close();
        qDebug() << "Created test CSV file:" << TEST_FILE;
    }
}

// 打印 CSV 内容
void printCsvContent(const QCsv& csv) {
    qDebug() << "\n=== CSV Content ===";
    
    // 测试一些已知的单元格
    QStringList testKeys = {"A1", "B2", "C3", "A4", "B5", "C6"};
    
    for (const QString& key : testKeys) {
        QString value = csv.getValue(key);
        if (!value.isEmpty()) {
            qDebug() << key << "=" << value;
        } else {
            qDebug() << key << "= (empty)";
        }
    }
    
    qDebug() << "=== End of Content ===\n";
}

// 测试基本功能
void testBasicFunctions() {
    qDebug() << "\n=== 测试基本功能 ===";
    
    // 测试1: 创建对象并打开文件
    QCsv csv(TEST_FILE, nullptr);
    qDebug() << "1. 创建 QCsv 对象";
    qDebug() << "   文件路径:" << TEST_FILE;
    qDebug() << "   是否打开:" << csv.isOpen();
    
    // 测试2: 加载数据
    csv.load();
    qDebug() << "2. 加载数据完成";
    qDebug() << "   分隔符:" << csv.getSeparator();
    
    // 测试3: 获取值
    QString value = csv.getValue("B2");
    qDebug() << "3. 获取单元格 B2 的值:" << value;
    
    // 测试4: 搜索功能
    QList<QString> searchResults = csv.search("2");
    qDebug() << "4. 搜索包含 '2' 的单元格:";
    for (const QString& result : searchResults) {
        qDebug() << "   -" << result;
    }
    
    // 测试5: 打印完整内容
    printCsvContent(csv);
}

// 测试修改功能
void testModificationFunctions() {
    qDebug() << "\n=== 测试修改功能 ===";
    
    // 创建新的 CSV 对象
    QCsv csv(TEST_FILE, nullptr);
    csv.load();
    
    // 测试1: 设置新值
    qDebug() << "1. 设置单元格 A1 的值为 'TestValue'";
    csv.setValue("A1", "TestValue");
    
    // 验证设置
    QString newValue = csv.getValue("A1");
    qDebug() << "   验证 A1 =" << newValue;
    
    // 测试2: 保存到新文件
    qDebug() << "2. 保存到新文件:" << TEST_FILE_SAVE;
    csv.saveAs(TEST_FILE_SAVE);
    
    // 测试3: 原子保存
    qDebug() << "3. 原子保存到:" << TEST_FILE_ATOMIC;
    csv.atomicSaveAs(TEST_FILE_ATOMIC);
    
    // 测试4: 验证保存的文件
    QCsv savedCsv(TEST_FILE_SAVE, nullptr);
    savedCsv.load();
    QString savedValue = savedCsv.getValue("A1");
    qDebug() << "   验证保存的文件中 A1 =" << savedValue;
    
    // 测试5: 清除数据
    qDebug() << "4. 清除数据";
    csv.clear();
    qDebug() << "   清除后获取 A1:" << csv.getValue("A1");
}

// 测试分隔符功能
void testSeparatorFunctions() {
    qDebug() << "\n=== 测试分隔符功能 ===";
    
    // 创建使用不同分隔符的 CSV 文件
    QString semicolonFile = "test_semicolon.csv";
    QFile file(semicolonFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "A1;B1;C1\n";
        out << "A2;B2;C2\n";
        out << "X;Y;Z\n";
        file.close();
    }
    
    QCsv csv(semicolonFile, nullptr);
    
    // 测试1: 默认分隔符
    qDebug() << "1. 默认分隔符:" << csv.getSeparator();
    
    // 测试2: 设置新分隔符
    csv.setSeparator(';');
    qDebug() << "2. 设置分隔符为 ';'";
    qDebug() << "   新分隔符:" << csv.getSeparator();
    
    // 测试3: 加载数据
    csv.load();
    
    // 验证数据是否正确加载
    QString value = csv.getValue("B2");
    qDebug() << "3. 使用分号分隔符加载数据";
    qDebug() << "   B2 的值:" << value;
    
    // 清理临时文件
    QFile::remove(semicolonFile);
}

// 测试文件操作
void testFileOperations() {
    qDebug() << "\n=== 测试文件操作 ===";
    
    // 测试1: 打开不存在的文件
    qDebug() << "1. 打开不存在的文件";
    QCsv csv1("nonexistent.csv", nullptr);
    qDebug() << "   文件状态:" << csv1.isOpen();
    
    // 测试2: 打开存在的文件
    qDebug() << "2. 打开存在的文件";
    QCsv csv2(TEST_FILE, nullptr);
    csv2.open(TEST_FILE);
    qDebug() << "   文件状态:" << csv2.isOpen();
    
    // 测试3: 加载和同步
    csv2.load();
    csv2.setValue("C3", "UpdatedValue");
    qDebug() << "3. 修改后同步到文件";
    csv2.sync();
    
    // 测试4: 关闭文件
    qDebug() << "4. 关闭文件";
    csv2.close();
    qDebug() << "   关闭后状态:" << csv2.isOpen();
    
    // 测试5: finalize 操作
    qDebug() << "5. 测试 finalize 操作";
    QCsv csv3(TEST_FILE, nullptr);
    csv3.load();
    csv3.setValue("A2", "FinalizedValue");
    csv3.finalize();
    qDebug() << "   finalize 完成";
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== QCsv 测试开始 ===";
    
    // 清理之前的测试文件
    cleanupTestFiles();
    
    // 创建测试文件
    createTestCsvFile();
    
    try {
        // 运行各个测试
        testBasicFunctions();
        testModificationFunctions();
        testSeparatorFunctions();
        testFileOperations();
        
        qDebug() << "\n=== 所有测试完成 ===";
        qDebug() << "测试文件已创建:";
        qDebug() << "1." << TEST_FILE << "(原始测试文件)";
        qDebug() << "2." << TEST_FILE_SAVE << "(保存测试文件)";
        qDebug() << "3." << TEST_FILE_ATOMIC << "(原子保存测试文件)";
        
        // 询问是否清理测试文件
        std::cout << "\n是否清理测试文件？(y/n): ";
        char choice;
        std::cin >> choice;
        
        if (choice == 'y' || choice == 'Y') {
            cleanupTestFiles();
            qDebug() << "已清理测试文件";
        }
        
    } catch (const std::exception& e) {
        qCritical() << "测试过程中发生异常:" << e.what();
        cleanupTestFiles();
        return 1;
    } catch (...) {
        qCritical() << "测试过程中发生未知异常";
        cleanupTestFiles();
        return 1;
    }
    
    return 0;
}