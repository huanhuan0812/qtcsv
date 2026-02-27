#pragma once
#include <QObject>
#include <QString>
#include <QHash>
#include <QMultiMap>
#include <QStringBuilder>
#include <memory>
#include <optional>
#include <QFuture>

// 导出宏定义
#if defined(QTCSV_SHARED)
    #if defined(_WIN32)
        #if defined(QTCSV_BUILDING_SHARED)
            #define QTCSV_EXPORT __declspec(dllexport)
        #else
            #define QTCSV_EXPORT __declspec(dllimport)
        #endif
    #else
        #if defined(QTCSV_BUILDING_SHARED)
            #define QTCSV_EXPORT __attribute__((visibility("default")))
        #else
            #define QTCSV_EXPORT
        #endif
    #endif
#else
    #define QTCSV_EXPORT
#endif

// 功能开关
#define EXPERIMENTAL_FUNC 0

// 键格式示例 "A1" 类似Excel

namespace CsvUtils {
    // 将数字转换为列字母 (0-based -> A, B, ..., Z, AA, AB, ...)
    inline QString numberToColumnRow(int number) {
        if (number < 0) return QString();
        
        QString column;
        int n = number + 1;
        while (n > 0) {
            int remainder = (n - 1) % 26;
            column.prepend(QChar('A' + remainder));
            n = (n - 1) / 26;
        }
        return column;
    }
    
    // 将列字母转换为数字 (A, B, ..., Z, AA, AB, ... -> 0-based)
    inline int columnRowToNumber(const QString& column) {
        int result = 0;
        for (QChar ch : column) {
            result = result * 26 + (ch.toUpper().toLatin1() - 'A' + 1);
        }
        return result - 1;
    }

    // 拆分键为列部分和行号
    inline std::pair<QString, int> splitKey(const QString& key) {
        int i = 0;
        while (i < key.length() && key[i].isLetter()) {
            ++i;
        }
        return {key.left(i), key.mid(i).toInt() - 1};
    }
    
    // 检查值是否需要引号包围
    inline bool needsQuotes(const QString& value, char separator) {
        return value.contains(separator) || 
               value.contains('"') || 
               value.contains('\n') || 
               value.contains('\r');
    }
    
    // 转义CSV值中的引号
    inline QString escapeQuotes(const QString& value) {
        QString result = value;
        result.replace('"', "\"\"");
        return result;
    }
}

// CSV解析器状态机
class CsvParser {
public:
    enum State {
        STATE_NORMAL,
        STATE_IN_QUOTES,
        STATE_QUOTE_IN_QUOTES,
        STATE_END_OF_CELL,
        STATE_END_OF_ROW
    };

    struct Statistics {
        int maxRow = 0;
        int maxCol = 0;
        int totalCells = 0;
        int emptyCells = 0;
    };

    CsvParser(QHash<QString, QString>& csvModel, 
              QMultiMap<QString, QString>& searchModel,
              char separator);

    void parse(const char* data, size_t size, bool isFinal = false);
    void finalize();
    
    // 获取统计信息
    const Statistics& getStatistics() const { return stats; }
    void resetStatistics();

private:
    QHash<QString, QString>& csvModel;
    QMultiMap<QString, QString>& searchModel;
    char separator;
    
    int currentRow = 0;
    int currentCol = 0;
    QString currentCell;
    State state = STATE_NORMAL;
    bool pendingCR = false;
    
    Statistics stats;
    
    void processChar(char ch);
    void endCell();
    void endRow();
    void insertCell();
};

// UTF-8感知的CSV解析器
class Utf8CsvParser {
public:
    struct Statistics {
        int maxRow = 0;
        int maxCol = 0;
        int totalCells = 0;
        int emptyCells = 0;
    };

    Utf8CsvParser(QHash<QString, QString>& csvModel, 
                  QMultiMap<QString, QString>& searchModel,
                  char separator);

    void parse(const char* data, size_t size, bool isFinal = false);
    void finalize();
    
    const Statistics& getStatistics() const { return stats; }
    void resetStatistics();

private:
    enum State {
        STATE_NORMAL,
        STATE_IN_QUOTES,
        STATE_QUOTE_IN_QUOTES,
        STATE_END_OF_CELL,
        STATE_END_OF_ROW
    };

    QHash<QString, QString>& csvModel;
    QMultiMap<QString, QString>& searchModel;
    char separator;
    
    int currentRow = 0;
    int currentCol = 0;
    QString currentCell;
    QByteArray utf8Buffer;  // 用于累积UTF-8字符的缓冲区
    State state = STATE_NORMAL;
    bool pendingCR = false;
    
    Statistics stats;
    
    void processUtf8Char(const char*& ptr, const char* end);
    void flushUtf8Char();
    void processChar(QChar ch);
    void endCell();
    void endRow();
    void insertCell();
};

class QTCSV_EXPORT QCsv : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString filePath READ getFilePath WRITE setFilePath)
    Q_PROPERTY(char separator READ getSeparator WRITE setSeparator)
    Q_PROPERTY(bool isOpen READ isOpen)
    Q_PROPERTY(int rowCount READ getRowCount)
    Q_PROPERTY(int columnCount READ getColumnCount)

public:
    explicit QCsv(const QString& filePath, QObject* parent = nullptr);
    ~QCsv();

    // 禁用拷贝，允许移动
    QCsv(const QCsv&) = delete;
    QCsv& operator=(const QCsv&) = delete;
    QCsv(QCsv&& other) noexcept;
    QCsv& operator=(QCsv&& other) noexcept;

    // 文件操作
    void open(const QString& filePath);
    void close();
    bool isOpen() const;
    
    // 数据加载和保存
    void load();
    bool save();
    bool saveAs(const QString& filePath);
    bool atomicSave();
    bool atomicSaveAs(const QString& filePath);
    void clear();
    
    // 异步操作
    QFuture<bool> sync();
    void finalize(); // 保存并关闭
    
    // 数据访问
    QString getValue(const QString& key) const;
    std::optional<QString> tryGetValue(const QString& key) const;
    void setValue(const QString& key, const QString& value);
    
    // 批量操作
    void setValues(const QHash<QString, QString>& values);
    QHash<QString, QString> getAllValues() const { return csvModel; }
    
    // 搜索功能
    QList<QString> search(const QString& value) const;
    QList<QString> searchByPrefix(const QString& prefix) const;
    
    // 属性访问
    void setSeparator(char sep);
    char getSeparator() const { return separator; }
    void setFilePath(const QString& path) { filePath = path; }
    QString getFilePath() const { return filePath; }
    
    // 获取行列信息
    int getRowCount() const { return maxRow; }
    int getColumnCount() const { return maxCol; }
    
    // 流式读取
    friend QCsv& operator>>(QCsv& csv, QString& value);
    
    // 检查单元格是否存在
    bool contains(const QString& key) const { return csvModel.contains(key); }
    
    // 获取所有键
    QList<QString> keys() const { return csvModel.keys(); }
    
    // 获取大小
    int size() const { return csvModel.size(); }
    bool isEmpty() const { return csvModel.isEmpty(); }

    void resetStream();

    bool hasNext() const;

    //-------------------------- 未经测试的未来功能 --------------------------

    // 头部处理
    void enableHeaders(bool enable);
    bool headersEnabled() const { return headersOn; }

    //列名称
    void setHeaderRow(int row=1);
    void setColumnHeader(int col, const QString& header);
    void setColumnHeaders(const QHash<int, QString>& headers);
    QList<int> searchColumnHeader(const QString& header) const;
    QString getColumnHeader(int col) const;
    QList<QString> getColumnHeaders() const ;
    QStringList getColumnHeaderLists() const;
    int getHeaderRow() const { return headerRow; }

    // 行名称
    void setHeaderColumn(int col=1);
    void setRowHeader(int row, const QString& header);
    void setRowHeaders(const QHash<int, QString>& headers);
    QList<int> searchRowHeader(const QString& header) const;
    QString getRowHeader(int row) const;
    QList<QString> getRowHeaders() const ;
    QStringList getRowHeaderLists() const;
    int getHeaderCol() const { return headerCol; }

    // 文件元数据
    QDateTime getLastModified() const;
    qint64 getFileSize() const;
    QMap<QString, QVariant> getAllMetadata() const;

    // 类型判断
    bool isNumeric(const QString& value) const;
    bool isDate(const QString& value, const QString& format = "yyyy-MM-dd") const;
    bool isBoolean(const QString& value) const;

    // 类型转换
    std::optional<double> toDouble(const QString& value) const;
    std::optional<QDate> toDate(const QString& value, const QString& format = "yyyy-MM-dd") const;
    std::optional<bool> toBoolean(const QString& value) const;

    //----------------------------- TODO: 未来功能 -----------------------------

    // 数据验证
    //bool validateCell(const QString& key, std::function<bool(const QString&)> validator) const;
    //bool validateAll(std::function<bool(const QString&, const QString&)> validator) const;

    

    //大文件处理
    //void loadLargeFile(const QString& filePath);
    //void appendToLargeFile(const QString& filePath, const QHash<QString, QString>& newData);

    // ------------------------------ 未来功能 -----------------------------


signals:
    void dataChanged(const QString& key, const QString& oldValue, const QString& newValue);
    void fileOpened(const QString& filePath);
    void fileClosed();
    void fileSaved(const QString& filePath);
    void error(const QString& errorString);

private:
    QString filePath;
    QHash<QString, QString> csvModel;
    QMultiMap<QString, QString> searchModel;
    char separator = ',';
    bool opened = false;
    int maxRow = 1;
    int maxCol = 1;
    bool headersOn = false;

    // 流式读取相关
    mutable int currentRow = 0;
    mutable int currentCol = 0;
    mutable QString currentCell;
    mutable CsvParser::State state = CsvParser::STATE_NORMAL;
    mutable bool pendingCR = false;
    mutable bool atEnd = false;

    std::unique_ptr<std::ifstream> fileStream;
    std::vector<char> buffer;
    size_t bufferPos = 0;
    size_t bufferSize = 0;
    const size_t BUFFER_SIZE = 16384; 

    // 头名称
    int headerRow = 1;
    int headerCol = 1;
    
    // 私有辅助方法
    void openStream();
    void closeStream();
    bool readNextCell(QString& result);
    void endCell();
    void endRow();
    bool getNextChar(char& ch);
    void appendToCurrentCell(char ch);
    void updateMaxRowCol(const QString& key);
    bool writeToStream(QTextStream& out) const;
    void removeFromSearch(const QString& value, const QString& key);
    
#if EXPERIMENTAL_FUNC
    bool seekToCell(int targetRow, int targetCol);
#endif
};