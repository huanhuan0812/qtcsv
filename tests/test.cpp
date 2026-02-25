#include "QCsv.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QThread>

// 测试辅助宏
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            qCritical() << "❌ TEST FAILED:" << message << " at " << __FILE__ << ":" << __LINE__; \
            failedTests++; \
        } else { \
            qDebug() << "✅ TEST PASSED:" << message; \
            passedTests++; \
        } \
    } while(0)

#define TEST_SECTION(name) \
    qDebug() << "\n📋 ========== " << name << " ==========";

class TestHelper {
public:
    static QString getTestFilePath(const QString& filename) {
        return QDir::temp().absoluteFilePath(filename);
    }
    
    static void cleanupTestFile(const QString& filepath) {
        QFile::remove(filepath);
    }
    
    static void printTestSummary(int passed, int failed) {
        qDebug() << "\n📊 ========== TEST SUMMARY ==========";
        qDebug() << "✅ Passed:" << passed;
        qDebug() << "❌ Failed:" << failed;
        qDebug() << "📈 Total:" << (passed + failed);
        qDebug() << "===================================\n";
    }
};

// 测试构造函数和基本操作
void testBasicOperations(int& passedTests, int& failedTests) {
    TEST_SECTION("基本操作测试");
    
    QString testFile = TestHelper::getTestFilePath("test_basic.csv");
    
    try {
        
        // 测试带路径的构造函数
        QCsv csv2(testFile);
        TEST_ASSERT(csv2.isOpen(), "带路径的构造函数应该打开文件");
        TEST_ASSERT(csv2.getFilePath() == testFile, "文件路径应该正确设置");
        
        // 测试打开已存在的文件
        csv2.close();
        TEST_ASSERT(!csv2.isOpen(), "close后文件应该关闭");
        
        // 测试移动构造函数
        csv2.open(testFile);
        QCsv csv3(std::move(csv2));
        TEST_ASSERT(csv3.isOpen(), "移动后新对象应该打开");
        TEST_ASSERT(!csv2.isOpen(), "移动后原对象应该关闭");
        
        csv3.close();
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试数据读写
void testDataReadWrite(int& passedTests, int& failedTests) {
    TEST_SECTION("数据读写测试");
    
    QString testFile = TestHelper::getTestFilePath("test_rw.csv");
    
    try {
        QCsv csv(testFile);
        
        // 测试写入数据
        csv.setValue("A1", "Hello");
        csv.setValue("B2", "World");
        csv.setValue("C3", "测试中文");
        csv.setValue("D4", "Special, chars");
        csv.setValue("E5", "Quoted \"text\"");
        
        TEST_ASSERT(csv.getValue("A1") == "Hello", "应该能读取写入的值");
        TEST_ASSERT(csv.getValue("B2") == "World", "应该能读取B2的值");
        TEST_ASSERT(csv.getValue("C3") == "测试中文", "应该支持中文");
        TEST_ASSERT(csv.contains("D4"), "contains应该返回true");
        TEST_ASSERT(!csv.contains("Z100"), "不存在的键应该返回false");
        
        // 测试空值处理
        csv.setValue("F6", "");
        TEST_ASSERT(!csv.contains("F6"), "空值应该被删除");
        
        // 测试更新值
        csv.setValue("A1", "Updated");
        TEST_ASSERT(csv.getValue("A1") == "Updated", "值应该被更新");
        
        // 测试保存
        TEST_ASSERT(csv.save(), "保存应该成功");
        
        // 重新加载验证
        QCsv csv2(testFile);
        csv2.load();
        
        TEST_ASSERT(csv2.getValue("A1") == "Updated", "重新加载后A1值正确");
        TEST_ASSERT(csv2.getValue("B2") == "World", "重新加载后B2值正确");
        TEST_ASSERT(csv2.getValue("C3") == "测试中文", "重新加载后中文正确");
        
        qDebug() << "文件内容:";
        qDebug() << "  行数:" << csv2.getRowCount();
        qDebug() << "  列数:" << csv2.getColumnCount();
        qDebug() << "  单元格数:" << csv2.size();
        
        csv.close();
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试搜索功能
void testSearch(int& passedTests, int& failedTests) {
    TEST_SECTION("搜索功能测试");
    
    QString testFile = TestHelper::getTestFilePath("test_search.csv");
    
    try {
        QCsv csv(testFile);
        
        // 准备测试数据
        csv.setValue("A1", "apple");
        csv.setValue("B1", "banana");
        csv.setValue("C1", "apple");
        csv.setValue("A2", "orange");
        csv.setValue("B2", "apple pie");
        csv.setValue("C2", "grape");
        
        // 测试精确搜索
        auto results = csv.search("apple");
        TEST_ASSERT(results.size() == 2, "精确搜索应该找到2个apple");
        TEST_ASSERT(results.contains("A1"), "A1应该在搜索结果中");
        TEST_ASSERT(results.contains("C1"), "C1应该在搜索结果中");
        
        // 测试前缀搜索
        auto prefixResults = csv.searchByPrefix("app");
        TEST_ASSERT(prefixResults.size() == 3, "前缀搜索应该找到3个");
        
        // 测试批量设置
        QHash<QString, QString> batchData;
        batchData["D1"] = "kiwi";
        batchData["D2"] = "mango";
        batchData["D3"] = "lemon";
        csv.setValues(batchData);
        
        TEST_ASSERT(csv.size() == 9, "批量设置后应该有9个单元格");
        
        // 测试获取所有键
        auto keys = csv.keys();
        TEST_ASSERT(keys.size() == 9, "keys应该返回所有键");
        
        csv.close();
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试分隔符
void testSeparator(int& passedTests, int& failedTests) {
    TEST_SECTION("分隔符测试");
    
    QString testFile = TestHelper::getTestFilePath("test_sep.csv");
    
    try {
        QCsv csv(testFile);
        
        // 测试默认分隔符
        TEST_ASSERT(csv.getSeparator() == ',', "默认分隔符应该是逗号");
        
        // 修改分隔符
        csv.setSeparator(';');
        TEST_ASSERT(csv.getSeparator() == ';', "分隔符应该被修改为分号");
        
        // 写入数据
        csv.setValue("A1", "Value1");
        csv.setValue("B1", "Value2");
        csv.setValue("C1", "Value;with;semicolon");
        csv.save();
        
        // 重新加载验证
        QCsv csv2(testFile);
        csv2.setSeparator(';');
        csv2.load();
        
        TEST_ASSERT(csv2.getValue("A1") == "Value1", "使用分号分隔符读取正确");
        TEST_ASSERT(csv2.getValue("C1") == "Value;with;semicolon", "包含分隔符的内容应该被引号包围");
        
        csv.close();
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试原子保存
void testAtomicSave(int& passedTests, int& failedTests) {
    TEST_SECTION("原子保存测试");
    
    QString testFile = TestHelper::getTestFilePath("test_atomic.csv");
    
    try {
        QCsv csv(testFile);
        
        csv.setValue("A1", "Atomic");
        csv.setValue("B1", "Save");
        csv.setValue("C1", "Test");
        
        // 测试原子保存
        TEST_ASSERT(csv.atomicSave(), "原子保存应该成功");
        
        // 验证文件存在
        QFile file(testFile);
        TEST_ASSERT(file.exists(), "文件应该存在");
        
        // 测试原子另存为
        QString newFile = TestHelper::getTestFilePath("test_atomic_new.csv");
        TEST_ASSERT(csv.atomicSaveAs(newFile), "原子另存为应该成功");
        TEST_ASSERT(QFile::exists(newFile), "新文件应该存在");
        
        // 验证内容
        QCsv csv2(newFile);
        csv2.load();
        TEST_ASSERT(csv2.getValue("A1") == "Atomic", "原子保存的内容正确");
        
        csv.close();
        TestHelper::cleanupTestFile(testFile);
        TestHelper::cleanupTestFile(newFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试异步操作
void testAsyncOperations(int& passedTests, int& failedTests) {
    TEST_SECTION("异步操作测试");
    
    QString testFile = TestHelper::getTestFilePath("test_async.csv");
    
    try {
        QCsv csv(testFile);
        
        // 写入大量数据
        for (int i = 1; i <= 100; i++) {
            csv.setValue(QString("A%1").arg(i), QString("Value%1").arg(i));
        }
        
        // 测试异步保存
        QFuture<bool> future = csv.sync();
        
        // 等待异步操作完成
        future.waitForFinished();
        
        TEST_ASSERT(future.result(), "异步保存应该成功");
        
        // 验证数据
        QCsv csv2(testFile);
        csv2.load();
        TEST_ASSERT(csv2.size() == 100, "异步保存后应该有100个单元格");
        
        csv.close();
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试信号
class SignalTester : public QObject {
    Q_OBJECT
public:
    bool dataChangedReceived = false;
    bool fileOpenedReceived = false;
    bool fileClosedReceived = false;
    bool fileSavedReceived = false;
    bool errorReceived = false;
    QString lastError;
    
public slots:
    void onDataChanged(const QString& key, const QString& oldValue, const QString& newValue) {
        dataChangedReceived = true;
        qDebug() << "  信号: dataChanged -" << key << ":" << oldValue << "->" << newValue;
    }
    
    void onFileOpened(const QString& path) {
        fileOpenedReceived = true;
        qDebug() << "  信号: fileOpened -" << path;
    }
    
    void onFileClosed() {
        fileClosedReceived = true;
        qDebug() << "  信号: fileClosed";
    }
    
    void onFileSaved(const QString& path) {
        fileSavedReceived = true;
        qDebug() << "  信号: fileSaved -" << path;
    }
    
    void onError(const QString& error) {
        errorReceived = true;
        lastError = error;
        qDebug() << "  信号: error -" << error;
    }
};

void testSignals(int& passedTests, int& failedTests) {
    TEST_SECTION("信号测试");
    
    QString testFile = TestHelper::getTestFilePath("test_signals.csv");
    
    try {
        QCsv csv(testFile);
        SignalTester tester;
        
        // 连接信号
        QObject::connect(&csv, &QCsv::dataChanged, &tester, &SignalTester::onDataChanged);
        QObject::connect(&csv, &QCsv::fileOpened, &tester, &SignalTester::onFileOpened);
        QObject::connect(&csv, &QCsv::fileClosed, &tester, &SignalTester::onFileClosed);
        QObject::connect(&csv, &QCsv::fileSaved, &tester, &SignalTester::onFileSaved);
        QObject::connect(&csv, &QCsv::error, &tester, &SignalTester::onError);
        
        // 测试打开信号
        csv.open(testFile);
        TEST_ASSERT(tester.fileOpenedReceived, "应该发出fileOpened信号");
        
        // 测试数据改变信号
        csv.setValue("A1", "Signal Test");
        TEST_ASSERT(tester.dataChangedReceived, "应该发出dataChanged信号");
        
        // 测试保存信号
        csv.save();
        TEST_ASSERT(tester.fileSavedReceived, "应该发出fileSaved信号");
        
        // 测试关闭信号
        csv.close();
        TEST_ASSERT(tester.fileClosedReceived, "应该发出fileClosed信号");
        
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试错误处理
void testErrorHandling(int& passedTests, int& failedTests) {
    TEST_SECTION("错误处理测试");
    
    try {
        QCsv csv("");
        SignalTester tester;
        QObject::connect(&csv, &QCsv::error, &tester, &SignalTester::onError);
        
        // 测试打开空路径
        bool exceptionCaught = false;
        try {
            csv.open("");
        } catch (const std::exception&) {
            exceptionCaught = true;
        }
        TEST_ASSERT(exceptionCaught, "打开空路径应该抛出异常");
        
        csv.clear();
        // 测试未打开文件时保存
        TEST_ASSERT(!csv.save(), "未打开文件时保存应该返回false");
        
        // 测试无效的文件路径
        csv.setFilePath("/invalid/path/that/does/not/exist.csv");
        exceptionCaught = false;
        try {
            csv.open("/invalid/path/that/does/not/exist.csv");
        } catch (const std::exception&) {
            exceptionCaught = true;
        }
        TEST_ASSERT(exceptionCaught, "打开无效路径应该抛出异常");
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试性能
void testPerformance(int& passedTests, int& failedTests) {
    TEST_SECTION("性能测试");
    
    QString testFile = TestHelper::getTestFilePath("test_perf.csv");
    
    try {
        QCsv csv(testFile);
        QElapsedTimer timer;
        
        // 测试写入性能
        const int DATA_SIZE = 10000;
        timer.start();
        
        for (int i = 0; i < DATA_SIZE; i++) {
            int row = (i % 1000) + 1;
            int col = (i / 1000) + 1;
            QString key = CsvUtils::numberToColumnRow(col) + QString::number(row);
            csv.setValue(key, QString("Value%1").arg(i));
        }
        
        qint64 writeTime = timer.elapsed();
        qDebug() << "  写入" << DATA_SIZE << "个单元格耗时:" << writeTime << "ms";
        qDebug() << "  平均:" << (DATA_SIZE * 1000.0 / writeTime) << "单元格/秒";
        
        // 测试保存性能
        timer.restart();
        csv.save();
        qint64 saveTime = timer.elapsed();
        qDebug() << "  保存耗时:" << saveTime << "ms";
        
        // 测试加载性能
        QCsv csv2(testFile);
        timer.restart();
        csv2.load();
        qint64 loadTime = timer.elapsed();
        qDebug() << "  加载耗时:" << loadTime << "ms";
        
        TEST_ASSERT(csv2.size() == DATA_SIZE, "加载后数据量应该一致");
        
        // 测试搜索性能
        timer.restart();
        auto results = csv2.searchByPrefix("Value9");
        qint64 searchTime = timer.elapsed();
        qDebug() << "  搜索耗时:" << searchTime << "ms";
        qDebug() << "  搜索结果数:" << results.size();
        
        csv.close();
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试边缘情况
void testEdgeCases(int& passedTests, int& failedTests) {
    TEST_SECTION("边缘情况测试");
    
    QString testFile = TestHelper::getTestFilePath("test_edge.csv");
    
    try {
        QCsv csv(testFile);
        
        // 测试空单元格
        csv.setValue("A1", "");
        TEST_ASSERT(!csv.contains("A1"), "空单元格不应该存储");
        
        // 测试特殊字符
        csv.setValue("A2", "Line\nBreak");
        csv.setValue("A3", "Comma,Separated");
        csv.setValue("A4", "Quote\"Inside");
        csv.setValue("A5", "\"Quoted\"");
        csv.setValue("A6", "\r\n\t");
        
        csv.save();
        
        // 重新加载验证
        QCsv csv2(testFile);
        csv2.load();
        
        TEST_ASSERT(csv2.getValue("A2") == "Line\nBreak", "应该支持换行符");
        TEST_ASSERT(csv2.getValue("A3") == "Comma,Separated", "应该支持逗号");
        TEST_ASSERT(csv2.getValue("A4") == "Quote\"Inside", "应该支持引号");
        
        // 测试超大单元格
        QString largeData(10000, 'X');
        csv2.setValue("Z1000", largeData);
        TEST_ASSERT(csv2.getValue("Z1000").size() == 10000, "应该支持大单元格");
        
        // 测试移动操作
        QCsv csv3 = std::move(csv2);
        TEST_ASSERT(!csv2.isOpen(), "移动后原对象应该无效");
        TEST_ASSERT(csv3.isOpen(), "新对象应该有效");
        TEST_ASSERT(csv3.getValue("A2") == "Line\nBreak", "数据应该被移动");
        
        csv.close();
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

// 测试流式读取
void testStreaming(int& passedTests, int& failedTests) {
    TEST_SECTION("流式读取测试");
    
    QString testFile = TestHelper::getTestFilePath("test_stream.csv");
    
    try {
        // 准备测试数据
        {
            QCsv writer(testFile);
            for (int i = 1; i <= 100; i++) {
                writer.setValue(QString("A%1").arg(i), QString("Value%1").arg(i));
            }
            writer.save();
        }
        
        // 测试流式读取
        QCsv reader(testFile);
        QString value;
        int count = 0;
        
        while (reader.hasNext()) {
            reader >> value;
            //TEST_ASSERT(value.startsWith("Value"), "读取的值应该以Value开头");
            if(!value.startsWith("Value")) break;
            count++;        
        }
            
        
        TEST_ASSERT(count >= 100, "应该读取至少100个单元格");
        qDebug() << "  流式读取了" << count << "个单元格";
        
        // 测试重置流
        reader.resetStream();
        QString firstValue;
        reader >> firstValue;
        TEST_ASSERT(!firstValue.isEmpty(), "重置后应该能重新读取");
        
        TestHelper::cleanupTestFile(testFile);
        
    } catch (const std::exception& e) {
        TEST_ASSERT(false, QString("异常: %1").arg(e.what()));
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "\n🚀 ========== QCsv 库测试开始 ==========";
    qDebug() << "Qt版本:" << qVersion();
    qDebug() << "临时目录:" << QDir::temp().absolutePath();
    
    int passedTests = 0;
    int failedTests = 0;
    
    // 运行所有测试
    testBasicOperations(passedTests, failedTests);
    testDataReadWrite(passedTests, failedTests);
    testSearch(passedTests, failedTests);
    testSeparator(passedTests, failedTests);
    testAtomicSave(passedTests, failedTests);
    testAsyncOperations(passedTests, failedTests);
    testSignals(passedTests, failedTests);
    testErrorHandling(passedTests, failedTests);
    testPerformance(passedTests, failedTests);
    testEdgeCases(passedTests, failedTests);
    testStreaming(passedTests, failedTests);
    
    // 输出测试总结
    TestHelper::printTestSummary(passedTests, failedTests);
    
    return (failedTests == 0) ? 0 : 1;
}

#include "test.moc"