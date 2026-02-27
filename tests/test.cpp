#include <QCoreApplication>
#include <QtTest/QTest>
#include <QDebug>
#include <QFile>
#include <QTemporaryFile>
#include <QDateTime>
#include <QSet>
#include "QCsv.hpp"

class QCsvTest : public QObject {
    Q_OBJECT

private:
    // 辅助方法：创建测试CSV文件
    QString createTestCsvFile() {
        QFile file("qtcsv_test_file.csv");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write("Name,Age,City\n");
            file.write("Alice,25,New York\n");
            file.write("Bob,30,Los Angeles\n");
            file.write("Charlie,35,Chicago\n");
            file.close();
            return file.fileName();
        }
        return QString();
    }

private slots:
    // 初始化测试环境
    void initTestCase() {
        qDebug() << "开始测试QCsv未测试代码...";
    }

    // 清理测试环境
    void cleanupTestCase() {
        qDebug() << "QCsv测试完成。";
    }

    // 每个测试用例前的设置
    void init() {
        // 可以在这里添加每个测试前的初始化代码
    }

    // 每个测试用例后的清理
    void cleanup() {
        // 可以在这里添加每个测试后的清理代码
    }

    // ==================== 测试 enableHeaders 方法 ====================
    void testEnableHeaders() {
        QCsv csv("test.csv");
        
        // 测试默认值
        QCOMPARE(csv.headersEnabled(), false);
        
        // 测试启用
        csv.enableHeaders(true);
        QCOMPARE(csv.headersEnabled(), true);
        
        // 测试重复启用相同值
        csv.enableHeaders(true);
        QCOMPARE(csv.headersEnabled(), true);
        
        // 测试禁用
        csv.enableHeaders(false);
        QCOMPARE(csv.headersEnabled(), false);
        
        // 测试重复禁用
        csv.enableHeaders(false);
        QCOMPARE(csv.headersEnabled(), false);
    }

    // ==================== 测试列标题相关方法 ====================
    void testColumnHeaders() {
        QString filePath = createTestCsvFile();
        QCsv csv(filePath);
        csv.open(filePath);
        csv.load();
        
        // 测试 setHeaderRow
        qDebug() << "测试 setHeaderRow...";
        QCOMPARE(csv.getHeaderRow(), 1); // 默认应该是1
        csv.setHeaderRow(2);
        QCOMPARE(csv.getHeaderRow(), 2);
        
        // 测试异常情况：行号小于1
        try {
            csv.setHeaderRow(0);
            QFAIL("Expected std::invalid_argument not thrown");
        } catch (const std::invalid_argument& e) {
            QVERIFY(e.what());
        }
        
        csv.setHeaderRow(1); // 恢复默认
        
        // 测试 setColumnHeader
        qDebug() << "测试 setColumnHeader...";
        csv.setColumnHeader(1, "NewHeader1");
        QCOMPARE(csv.getColumnHeader(1), "NewHeader1");
        
        // 测试清除列标题
        csv.setColumnHeader(1, "");
        QCOMPARE(csv.getColumnHeader(1), QString());
        
        // 测试异常情况：列号小于1
        try {
            csv.setColumnHeader(0, "Invalid");
            QFAIL("Expected std::invalid_argument not thrown");
        } catch (const std::invalid_argument& e) {
            QVERIFY(e.what());
        }
        
        // 测试 setColumnHeaders 批量设置
        qDebug() << "测试 setColumnHeaders 批量设置...";
        QHash<int, QString> headers;
        headers.insert(1, "Header1");
        headers.insert(2, "Header2");
        headers.insert(3, "Header3");
        
        csv.setColumnHeaders(headers);
        
        QCOMPARE(csv.getColumnHeader(1), "Header1");
        QCOMPARE(csv.getColumnHeader(2), "Header2");
        QCOMPARE(csv.getColumnHeader(3), "Header3");
        
        // 测试批量设置包含空值
        headers.clear();
        headers.insert(1, "");
        headers.insert(2, "Header2");
        csv.setColumnHeaders(headers);
        QCOMPARE(csv.getColumnHeader(1), QString());
        QCOMPARE(csv.getColumnHeader(2), "Header2");
        
        // 测试异常情况：批量设置中的列号无效
        QHash<int, QString> invalidHeaders;
        invalidHeaders.insert(0, "Invalid");
        try {
            csv.setColumnHeaders(invalidHeaders);
            QFAIL("Expected std::invalid_argument not thrown");
        } catch (const std::invalid_argument& e) {
            QVERIFY(e.what());
        }
        
        // 测试 getColumnHeaders
        qDebug() << "测试 getColumnHeaders...";
        QList<QString> headersList = csv.getColumnHeaders();
        QCOMPARE(headersList.size(), csv.getColumnCount());
        
        // 测试 getColumnHeaderLists
        QStringList headerStrList = csv.getColumnHeaderLists();
        QCOMPARE(headerStrList.size(), csv.getColumnCount());
        
        // 测试 searchColumnHeader（返回多个结果）
        qDebug() << "测试 searchColumnHeader 返回多个结果...";
        csv.setColumnHeader(1, "SearchHeader1");
        csv.setColumnHeader(2, "SearchHeader2");
        csv.setColumnHeader(3, "SearchHeader1");  // 重复的标题
        csv.setColumnHeader(5, "SearchHeader1");  // 另一个重复
        
        QList<int> results = csv.searchColumnHeader("SearchHeader1");
        qDebug() << "找到的列号:" << results;
        QCOMPARE(results.size(), 3);
        QVERIFY(results.contains(1));
        QVERIFY(results.contains(3));
        QVERIFY(results.contains(5));
        QCOMPARE(results[0], 1);  // 排序后第一个是1
        QCOMPARE(results[1], 3);  // 排序后第二个是3
        QCOMPARE(results[2], 5);  // 排序后第三个是5
        
        results = csv.searchColumnHeader("SearchHeader2");
        QCOMPARE(results.size(), 1);
        QCOMPARE(results[0], 2);
        
        results = csv.searchColumnHeader("NonExistentHeader");
        QCOMPARE(results.size(), 0);
        
        // 测试空标题搜索
        results = csv.searchColumnHeader("");
        QCOMPARE(results.size(), 0);
    }

    // ==================== 测试行标题相关方法 ====================
    void testRowHeaders() {
        QString filePath = createTestCsvFile();
        QCsv csv(filePath);
        csv.open(filePath);
        csv.load();
        
        // 测试 setHeaderColumn
        qDebug() << "测试 setHeaderColumn...";
        QCOMPARE(csv.getHeaderCol(), 1); // 默认应该是1
        csv.setHeaderColumn(2);
        QCOMPARE(csv.getHeaderCol(), 2);
        
        // 测试异常情况：列号小于1
        try {
            csv.setHeaderColumn(0);
            QFAIL("Expected std::invalid_argument not thrown");
        } catch (const std::invalid_argument& e) {
            QVERIFY(e.what());
        }
        
        csv.setHeaderColumn(1); // 恢复默认
        
        // 测试 setRowHeader
        qDebug() << "测试 setRowHeader...";
        csv.setRowHeader(1, "NewRowHeader1");
        QCOMPARE(csv.getRowHeader(1), "NewRowHeader1");
        
        // 测试清除行标题
        csv.setRowHeader(1, "");
        QCOMPARE(csv.getRowHeader(1), QString());
        
        // 测试异常情况：行号小于1
        try {
            csv.setRowHeader(0, "Invalid");
            QFAIL("Expected std::invalid_argument not thrown");
        } catch (const std::invalid_argument& e) {
            QVERIFY(e.what());
        }
        
        // 测试 setRowHeaders 批量设置
        qDebug() << "测试 setRowHeaders 批量设置...";
        QHash<int, QString> headers;
        headers.insert(1, "RowHeader1");
        headers.insert(2, "RowHeader2");
        headers.insert(3, "RowHeader3");
        
        csv.setRowHeaders(headers);
        
        QCOMPARE(csv.getRowHeader(1), "RowHeader1");
        QCOMPARE(csv.getRowHeader(2), "RowHeader2");
        QCOMPARE(csv.getRowHeader(3), "RowHeader3");
        
        // 测试批量设置包含空值
        headers.clear();
        headers.insert(1, "");
        headers.insert(2, "RowHeader2");
        csv.setRowHeaders(headers);
        QCOMPARE(csv.getRowHeader(1), QString());
        QCOMPARE(csv.getRowHeader(2), "RowHeader2");
        
        // 测试异常情况：批量设置中的行号无效
        QHash<int, QString> invalidHeaders;
        invalidHeaders.insert(0, "Invalid");
        try {
            csv.setRowHeaders(invalidHeaders);
            QFAIL("Expected std::invalid_argument not thrown");
        } catch (const std::invalid_argument& e) {
            QVERIFY(e.what());
        }
        
        // 测试 getRowHeaders
        qDebug() << "测试 getRowHeaders...";
        QList<QString> headersList = csv.getRowHeaders();
        QCOMPARE(headersList.size(), csv.getRowCount());
        
        // 测试 getRowHeaderLists
        QStringList headerStrList = csv.getRowHeaderLists();
        QCOMPARE(headerStrList.size(), csv.getRowCount());
        
        // 测试 searchRowHeader（返回多个结果）
        qDebug() << "测试 searchRowHeader 返回多个结果...";
        csv.setRowHeader(1, "SearchRowHeader1");
        csv.setRowHeader(2, "SearchRowHeader2");
        csv.setRowHeader(4, "SearchRowHeader1");  // 重复的标题
        csv.setRowHeader(6, "SearchRowHeader1");  // 另一个重复
        
        QList<int> results = csv.searchRowHeader("SearchRowHeader1");
        qDebug() << "找到的行号:" << results;
        QCOMPARE(results.size(), 3);
        QVERIFY(results.contains(1));
        QVERIFY(results.contains(4));
        QVERIFY(results.contains(6));
        QCOMPARE(results[0], 1);  // 排序后第一个是1
        QCOMPARE(results[1], 4);  // 排序后第二个是4
        QCOMPARE(results[2], 6);  // 排序后第三个是6
        
        results = csv.searchRowHeader("SearchRowHeader2");
        QCOMPARE(results.size(), 1);
        QCOMPARE(results[0], 2);
        
        results = csv.searchRowHeader("NonExistentHeader");
        QCOMPARE(results.size(), 0);
        
        // 测试空标题搜索
        results = csv.searchRowHeader("");
        QCOMPARE(results.size(), 0);
    }

    // ==================== 测试文件元数据相关方法 ====================
    void testFileMetadata() {
        QString filePath = createTestCsvFile();
        QCsv csv(filePath);
        
        // 测试空文件路径时的异常
        qDebug() << "测试空文件路径异常...";
        try {
            QCsv emptyCsv("");
            emptyCsv.getLastModified();
            QFAIL("Expected std::runtime_error not thrown");
        } catch (const std::runtime_error& e) {
            QVERIFY(e.what());
        }
        
        try {
            QCsv emptyCsv("");
            emptyCsv.getFileSize();
            QFAIL("Expected std::runtime_error not thrown");
        } catch (const std::runtime_error& e) {
            QVERIFY(e.what());
        }
        
        try {
            QCsv emptyCsv("");
            emptyCsv.getAllMetadata();
            QFAIL("Expected std::runtime_error not thrown");
        } catch (const std::runtime_error& e) {
            QVERIFY(e.what());
        }
        
        // 测试 getLastModified
        qDebug() << "测试 getLastModified...";
        QDateTime lastModified = csv.getLastModified();
        QVERIFY(lastModified.isValid());
        qDebug() << "文件最后修改时间:" << lastModified;
        
        // 测试 getFileSize
        qDebug() << "测试 getFileSize...";
        qint64 fileSize = csv.getFileSize();
        qDebug() <<csv.getFilePath();
        QVERIFY(fileSize > 0);
        qDebug() << "文件大小:" << fileSize << "bytes";
        
        // 测试 getAllMetadata
        qDebug() << "测试 getAllMetadata...";
        QMap<QString, QVariant> metadata = csv.getAllMetadata();
        QVERIFY(metadata.contains("lastModified"));
        QVERIFY(metadata.contains("fileSize"));
        QVERIFY(metadata.contains("created"));
        QVERIFY(metadata.contains("isReadable"));
        QVERIFY(metadata.contains("isWritable"));
        QVERIFY(metadata.contains("isExecutable"));
        
        QCOMPARE(metadata["fileSize"].toLongLong(), fileSize);
        QCOMPARE(metadata["lastModified"].toDateTime(), lastModified);
        
        // 打印所有元数据
        qDebug() << "文件元数据:";
        for (auto it = metadata.begin(); it != metadata.end(); ++it) {
            qDebug() << "  " << it.key() << ":" << it.value();
        }
    }

    // ==================== 测试类型判断方法 ====================
    void testTypeCheckMethods() {
        QCsv csv("test.csv");
        
        // 测试 isNumeric
        qDebug() << "测试 isNumeric...";
        QVERIFY(csv.isNumeric("123"));
        QVERIFY(csv.isNumeric("123.456"));
        QVERIFY(csv.isNumeric("-123.456"));
        QVERIFY(csv.isNumeric("0"));
        QVERIFY(csv.isNumeric("  123  "));  // 带空格的数字
        QVERIFY(!csv.isNumeric("abc"));
        QVERIFY(!csv.isNumeric("123abc"));
        QVERIFY(!csv.isNumeric(""));
        QVERIFY(!csv.isNumeric("12.34.56"));
        
        // 测试 isDate
        qDebug() << "测试 isDate...";
        QVERIFY(csv.isDate("2023-12-25", "yyyy-MM-dd"));
        QVERIFY(csv.isDate("2023/12/25", "yyyy/MM/dd"));
        QVERIFY(csv.isDate("25-12-2023", "dd-MM-yyyy"));
        QVERIFY(csv.isDate("2023.12.25", "yyyy.MM.dd"));
        QVERIFY(!csv.isDate("2023-13-25", "yyyy-MM-dd"));
        QVERIFY(!csv.isDate("2023-12-32", "yyyy-MM-dd"));
        QVERIFY(!csv.isDate("abc", "yyyy-MM-dd"));
        QVERIFY(!csv.isDate("", "yyyy-MM-dd"));
        
        // 测试默认格式
        QVERIFY(csv.isDate("2023-12-25"));  // 使用默认格式
        
        // 测试 isBoolean
        qDebug() << "测试 isBoolean...";
        QVERIFY(csv.isBoolean("true"));
        QVERIFY(csv.isBoolean("TRUE"));
        QVERIFY(csv.isBoolean("True"));
        QVERIFY(csv.isBoolean("false"));
        QVERIFY(csv.isBoolean("FALSE"));
        QVERIFY(csv.isBoolean("False"));
        QVERIFY(csv.isBoolean("1"));
        QVERIFY(csv.isBoolean("0"));
        QVERIFY(csv.isBoolean("  true  "));  // 带空格的布尔值
        QVERIFY(!csv.isBoolean("yes"));
        QVERIFY(!csv.isBoolean("no"));
        QVERIFY(!csv.isBoolean("2"));
        QVERIFY(!csv.isBoolean(""));
        QVERIFY(!csv.isBoolean("truefalse"));
    }

    // ==================== 测试类型转换方法 ====================
    void testTypeConversionMethods() {
        QCsv csv("test.csv");
        
        // 测试 toDouble
        qDebug() << "测试 toDouble...";
        auto doubleResult = csv.toDouble("123.456");
        QVERIFY(doubleResult.has_value());
        QCOMPARE(doubleResult.value(), 123.456);
        
        doubleResult = csv.toDouble("-789.123");
        QVERIFY(doubleResult.has_value());
        QCOMPARE(doubleResult.value(), -789.123);
        
        doubleResult = csv.toDouble("0");
        QVERIFY(doubleResult.has_value());
        QCOMPARE(doubleResult.value(), 0.0);
        
        doubleResult = csv.toDouble("  456.789  ");  // 带空格的数字
        QVERIFY(doubleResult.has_value());
        QCOMPARE(doubleResult.value(), 456.789);
        
        doubleResult = csv.toDouble("abc");
        QVERIFY(!doubleResult.has_value());
        
        doubleResult = csv.toDouble("123abc");
        QVERIFY(!doubleResult.has_value());
        
        doubleResult = csv.toDouble("");
        QVERIFY(!doubleResult.has_value());
        
        // 测试 toDate
        qDebug() << "测试 toDate...";
        auto dateResult = csv.toDate("2023-12-25", "yyyy-MM-dd");
        QVERIFY(dateResult.has_value());
        QCOMPARE(dateResult.value(), QDate(2023, 12, 25));
        
        dateResult = csv.toDate("2023/12/25", "yyyy/MM/dd");
        QVERIFY(dateResult.has_value());
        QCOMPARE(dateResult.value(), QDate(2023, 12, 25));
        
        dateResult = csv.toDate("25-12-2023", "dd-MM-yyyy");
        QVERIFY(dateResult.has_value());
        QCOMPARE(dateResult.value(), QDate(2023, 12, 25));
        
        // 测试默认格式
        dateResult = csv.toDate("2023-12-25");
        QVERIFY(dateResult.has_value());
        QCOMPARE(dateResult.value(), QDate(2023, 12, 25));
        
        dateResult = csv.toDate("2023-13-25", "yyyy-MM-dd");
        QVERIFY(!dateResult.has_value());
        
        dateResult = csv.toDate("2023-12-32", "yyyy-MM-dd");
        QVERIFY(!dateResult.has_value());
        
        dateResult = csv.toDate("abc", "yyyy-MM-dd");
        QVERIFY(!dateResult.has_value());
        
        dateResult = csv.toDate("");
        QVERIFY(!dateResult.has_value());
        
        // 测试 toBoolean
        qDebug() << "测试 toBoolean...";
        auto boolResult = csv.toBoolean("true");
        QVERIFY(boolResult.has_value());
        QCOMPARE(boolResult.value(), true);
        
        boolResult = csv.toBoolean("TRUE");
        QVERIFY(boolResult.has_value());
        QCOMPARE(boolResult.value(), true);
        
        boolResult = csv.toBoolean("  True  ");  // 带空格的布尔值
        QVERIFY(boolResult.has_value());
        QCOMPARE(boolResult.value(), true);
        
        boolResult = csv.toBoolean("false");
        QVERIFY(boolResult.has_value());
        QCOMPARE(boolResult.value(), false);
        
        boolResult = csv.toBoolean("FALSE");
        QVERIFY(boolResult.has_value());
        QCOMPARE(boolResult.value(), false);
        
        boolResult = csv.toBoolean("False");
        QVERIFY(boolResult.has_value());
        QCOMPARE(boolResult.value(), false);
        
        boolResult = csv.toBoolean("1");
        QVERIFY(boolResult.has_value());
        QCOMPARE(boolResult.value(), true);
        
        boolResult = csv.toBoolean("0");
        QVERIFY(boolResult.has_value());
        QCOMPARE(boolResult.value(), false);
        
        boolResult = csv.toBoolean("yes");
        QVERIFY(!boolResult.has_value());
        
        boolResult = csv.toBoolean("no");
        QVERIFY(!boolResult.has_value());
        
        boolResult = csv.toBoolean("");
        QVERIFY(!boolResult.has_value());
        
        boolResult = csv.toBoolean("2");
        QVERIFY(!boolResult.has_value());
    }

    // ==================== 测试组合场景 ====================
    void testCombinedScenarios() {
        QString filePath = createTestCsvFile();
        QCsv csv(filePath);
        csv.load();
        
        qDebug() << "测试组合场景...";

        // 场景1：设置列标题和行标题，然后搜索
        csv.setColumnHeader(1, "ID");
        csv.setColumnHeader(2, "Name");
        csv.setColumnHeader(3, "Age");

        
        //csv.setRowHeader(1, "Record1");
        csv.setRowHeader(2, "Record2");
        csv.setRowHeader(3, "Record3");
        
        // 验证列标题搜索
        QList<int> colResults = csv.searchColumnHeader("Name");
        QCOMPARE(colResults.size(), 1);
        QCOMPARE(colResults[0], 2);
        
        // 验证行标题搜索
        QList<int> rowResults = csv.searchRowHeader("Record2");
        QCOMPARE(rowResults.size(), 1);
        QCOMPARE(rowResults[0], 2);
        
        // 场景2：设置重复标题并搜索
        csv.setColumnHeader(4, "ID");  // 重复的ID列
        colResults = csv.searchColumnHeader("ID");
        QCOMPARE(colResults.size(), 2);
        QVERIFY(colResults.contains(1));
        QVERIFY(colResults.contains(4));
        
        // 场景3：清除标题后搜索
        csv.setColumnHeader(1, "");
        colResults = csv.searchColumnHeader("ID");
        QCOMPARE(colResults.size(), 1);
        QCOMPARE(colResults[0], 4);
        
        // 场景4：类型转换与标题结合
        QString ageValue = csv.getValue("B3");  // 应该是 "30"
        auto age = csv.toDouble(ageValue);
        QVERIFY(age.has_value());
        QCOMPARE(age.value(), 30.0);
        
        // 验证数据类型判断
        QVERIFY(csv.isNumeric(ageValue));
        QVERIFY(!csv.isBoolean(ageValue));
    }

    // ==================== 测试边界情况 ====================
    void testEdgeCases() {
        QCsv csv("test.csv");
        
        qDebug() << "测试边界情况...";
        
        // 测试极值
        auto doubleResult = csv.toDouble("999999999999999999");
        QVERIFY(doubleResult.has_value());
        
        doubleResult = csv.toDouble("-999999999999999999");
        QVERIFY(doubleResult.has_value());
        
        // 测试空字符串
        QVERIFY(!csv.isNumeric(""));
        QVERIFY(!csv.isDate(""));
        QVERIFY(!csv.isBoolean(""));
        
        // 测试特殊字符
        QVERIFY(!csv.isNumeric("$123"));
        QVERIFY(!csv.isNumeric("12.34.56"));
        
        // 测试超大日期（Qt支持的范围）
        auto dateResult = csv.toDate("9999-12-31");
        QVERIFY(dateResult.has_value());
        QCOMPARE(dateResult.value(), QDate(9999, 12, 31));
    }
};

QTEST_MAIN(QCsvTest)
#include "test.moc"