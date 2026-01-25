#pragma once
#include <QObject>
#include <QString>
#include <QHash>
#include <QMultiMap>

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
#elif defined(QTCSV_STATIC)
    #define QTCSV_EXPORT
#else
    // 默认情况下，当作静态库处理
    #define QTCSV_EXPORT
#endif

#define EXPERIMENTAL_FUNC 0

//key example "A1" like in Excel


// 添加静态辅助函数
namespace CsvUtils {
    static QString numberToColumnRow(int number) {
        QString column;
        number++;
        while (number > 0) {
            int colNum = (number - 1) % 26;
            column.prepend(QChar('A' + colNum));
            number = (number - 1) / 26;
        }
        return column;
    }
    
    static int columnRowToNumber(const QString& column) {
        int col = 0;
        for (int i = 0; i < column.length(); ++i) {
            col = col * 26 + (column[i].toUpper().toLatin1() - 'A' + 1);
        }
        return col - 1; // 转为0-based
    }
}

// CSV解析状态机
class CsvParser {
public:
    enum State {
        STATE_NORMAL,
        STATE_IN_QUOTES,
        STATE_QUOTE_IN_QUOTES,
        STATE_END_OF_CELL,
        STATE_END_OF_ROW
    };

    CsvParser(QHash<QString, QString>& csvModel, 
              QMultiMap<QString, QString>& searchModel,
              char separator)
        : csvModel(csvModel), searchModel(searchModel), separator(separator) {}

    void parse(const char* data, size_t size, bool isFinal = false);
    void finalize();

private:
    QHash<QString, QString>& csvModel;
    QMultiMap<QString, QString>& searchModel;
    char separator;
    
    int currentRow = 0;
    int currentCol = 0;
    QString currentCell;
    State state = STATE_NORMAL;
    bool pendingCR = false; // 处理\r\n换行符
    
    void processChar(char ch);
    void endCell();
    void endRow();
    
    void insertCell();
    
    // 用于跟踪最大行列
    int maxRow = 0;
    int maxCol = 0;
};

class QTCSV_EXPORT QCsv : public QObject {
    Q_OBJECT
public:
    QCsv(QString filePath, QObject* parent = nullptr);
    ~QCsv();

    QCsv(QCsv&& other) noexcept;
    QCsv& operator=(QCsv&& other) noexcept;

    void open(QString filePath);
    void close();
    bool isOpen() const;

    void load();
    void save();
    void saveAs(QString filePath);
    void clear();
    void sync();
    void atomicSave();
    void atomicSaveAs(QString filePath); // 原子保存

    void finalize(); // 保存并关闭文件

    QString getValue(const QString& key) const;
    QList<QString> search(QString index) const;

    void setValue(const QString& key, const QString& value);

    void setSeparator(char sep);
    char getSeparator() const;

    QString getFilePath() const { return filePath; }
    
    friend QCsv& operator>>(QCsv& csv, QString& value);
        
    


private:
    QString filePath;
    QHash<QString, QString> csvModel;
    QMultiMap<QString, QString> searchModel;
    char separator = ',';
    bool opened = false;

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
    
    // 用于流式读取的辅助函数
    void openStream();
    void closeStream();
    void resetStream();
    bool readNextCell(QString& result);
    inline void endCell() const;
    inline void endRow() const;
    bool getNextChar(char& ch);
    void apppendToCurrentCell(char ch);

    //TODO: add these functions if needed

    bool seekToCell(int targetRow, int targetCol);
    //bool readCell(QString& result) const;
    
};