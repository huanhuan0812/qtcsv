#include "QCsv.hpp"
#include <fstream>
#include <QDebug>
#include <stdexcept>
#include <algorithm>
#include <cstring>

QString QCsv::numberToColumnRow(int number) const {
    QString column;
    number++; // 从0-based转为1-based（A=1）
    while (number > 0) {
        int colNum = (number - 1) % 26;
        column.prepend(QChar('A' + colNum));
        number = (number - 1) / 26;
    }
    return column;
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
    QString numberToColumnRow(int number) const;
    
    // 用于跟踪最大行列
    int maxRow = 0;
    int maxCol = 0;
};

QString CsvParser::numberToColumnRow(int number) const {
    QString column;
    number++; // 从0-based转为1-based（A=1）
    while (number > 0) {
        int colNum = (number - 1) % 26;
        column.prepend(QChar('A' + colNum));
        number = (number - 1) / 26;
    }
    return column;
}

void CsvParser::processChar(char ch) {
    switch (state) {
        case STATE_NORMAL:
            if (ch == '"') {
                state = STATE_IN_QUOTES;
            } else if (ch == separator) {
                endCell();
            } else if (ch == '\n') {
                if (pendingCR) {
                    // \r\n组合，跳过
                    pendingCR = false;
                } else {
                    endCell();
                    endRow();
                }
            } else if (ch == '\r') {
                pendingCR = true;
                endCell();
                endRow();
            } else {
                currentCell.append(ch);
            }
            break;
            
        case STATE_IN_QUOTES:
            if (ch == '"') {
                state = STATE_QUOTE_IN_QUOTES;
            } else {
                currentCell.append(ch);
            }
            break;
            
        case STATE_QUOTE_IN_QUOTES:
            if (ch == '"') {
                // 双引号表示一个引号字符
                currentCell.append('"');
                state = STATE_IN_QUOTES;
            } else if (ch == separator) {
                endCell();
                state = STATE_NORMAL;
            } else if (ch == '\n') {
                endCell();
                endRow();
                state = STATE_NORMAL;
                pendingCR = false;
            } else if (ch == '\r') {
                pendingCR = true;
                endCell();
                endRow();
                state = STATE_NORMAL;
            } else {
                // 引号结束
                currentCell.append(ch);
                state = STATE_NORMAL;
            }
            break;
    }
}

void CsvParser::endCell() {
    // 记录最大列
    maxCol = std::max(maxCol, currentCol);
    
    if (!currentCell.isEmpty()) {
        insertCell();
    }
    currentCell.clear();
    currentCol++;
}

void CsvParser::endRow() {
    if (pendingCR) {
        // 处理单独的\r作为行结束
        if (!currentCell.isEmpty()) {
            endCell();
        }
        pendingCR = false;
    }
    
    // 记录最大行
    maxRow = std::max(maxRow, currentRow);
    
    currentRow++;
    currentCol = 0;
}

void CsvParser::insertCell() {
    QString key = QString("%1%2").arg(
        this->numberToColumnRow(currentCol)).arg(currentRow + 1);
    
    // 只在单元格不为空时才存储
    if (!currentCell.isEmpty()) {
        csvModel.insert(key, currentCell);
        // 同时添加到搜索索引
        searchModel.insert(currentCell, key);
    }
    // 空单元格不会存储在csvModel中
}

void CsvParser::parse(const char* data, size_t size, bool isFinal) {
    for (size_t i = 0; i < size; ++i) {
        processChar(data[i]);
    }
    
    if (isFinal) {
        finalize();
    }
}

void CsvParser::finalize() {
    if (state == STATE_IN_QUOTES) {
        // 文件在引号中结束，可能格式有问题，但还是处理
        qWarning() << "CSV file ended inside quoted field";
    }
    
    if (!currentCell.isEmpty()) {
        endCell();
    }
    
    if (currentCol > 0) {
        // 最后一行以非换行符结束
        endRow();
    }
}

void QCsv::load() {
    if (!opened || filePath.isEmpty()) {
        throw std::runtime_error("File not opened");
    }
    
    std::ifstream file(filePath.toStdString(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filePath.toStdString());
    }
    
    // 清空现有数据
    csvModel.clear();
    searchModel.clear();
    
    // 获取文件大小
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    const size_t CHUNK_SIZE = 1024 * 1024; // 1MB
    std::vector<char> buffer(CHUNK_SIZE);
    
    CsvParser parser(csvModel, searchModel, separator);
    
    while (file) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytesRead = file.gcount();
        
        if (bytesRead > 0) {
            parser.parse(buffer.data(), bytesRead, false);
        }
    }
    
    parser.finalize();
    file.close();
    
    qDebug() << "Loaded" << csvModel.size() << "cells from CSV";
}

// 添加其他成员函数的实现
QString QCsv::getValue(const QString& key) const {
    // 如果键不存在，返回空字符串（表示空单元格）
    return csvModel.value(key, "");
}

QList<QString> QCsv::search(QString value) const {
    return searchModel.values(value);
}

void QCsv::setValue(const QString& key, const QString& value) {
    // 获取旧值
    QString oldValue = csvModel.value(key);
    
    if (value.isEmpty()) {
        // 如果要设置的是空值，则从csvModel中删除该键
        if (!oldValue.isEmpty()) {
            csvModel.remove(key);
            // 同时从搜索索引中移除
            searchModel.remove(oldValue, key);
        }
    } else {
        // 如果新值非空
        if (!oldValue.isEmpty()) {
            // 有旧值，先从搜索索引中移除
            searchModel.remove(oldValue, key);
        }
        // 设置新值
        csvModel.insert(key, value);
        // 添加到搜索索引
        searchModel.insert(value, key);
    }
}

bool QCsv::isOpen() const {
    return opened && !filePath.isEmpty();
}

void QCsv::save() {
    if (!isOpen()) {
        throw std::runtime_error("No file opened for saving");
    }
    saveAs(filePath);
}

void QCsv::saveAs(QString newFilePath) {
    // 找出最大行和列
    int maxRow = 0;
    int maxCol = 0;
    
    for (auto it = csvModel.begin(); it != csvModel.end(); ++it) {
        QString key = it.key();
        // 解析Excel样式的键，如"A1"
        QString colStr;
        int row = 0;
        
        for (int i = 0; i < key.length(); ++i) {
            if (key[i].isLetter()) {
                colStr.append(key[i]);
            } else if (key[i].isDigit()) {
                row = row * 10 + key[i].digitValue();
            }
        }
        
        maxRow = std::max(maxRow, row);
        
        // 转换列字母为数字
        int col = 0;
        for (int i = 0; i < colStr.length(); ++i) {
            col = col * 26 + (colStr[i].toLatin1() - 'A' + 1);
        }
        maxCol = std::max(maxCol, col);
    }
    
    // 如果csvModel为空，写入空文件
    if (csvModel.isEmpty() && maxRow == 0 && maxCol == 0) {
        maxRow = 1;
        maxCol = 1;
    }
    
    // 写入CSV文件
    std::ofstream file(newFilePath.toStdString());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }
    
    for (int row = 1; row <= maxRow; ++row) {
        for (int col = 1; col <= maxCol; ++col) {
            // 转换列号回字母
            QString colStr = numberToColumnRow(col - 1);
            QString key = colStr + QString::number(row);
            
            // 获取单元格值（如果不存在于csvModel中，则为空）
            QString value = csvModel.value(key, "");
            
            // 需要引号的情况：包含分隔符、引号或换行符
            bool needsQuotes = value.contains(separator) || 
                              value.contains('"') || 
                              value.contains('\n') || 
                              value.contains('\r');
            
            if (needsQuotes) {
                // 转义引号
                value.replace("\"", "\"\"");
                file << "\"" << value.toStdString() << "\"";
            } else {
                file << value.toStdString();
            }
            
            if (col < maxCol) {
                file << separator;
            }
        }
        file << "\n";
    }
    
    file.close();
}

void QCsv::clear() {
    csvModel.clear();
    searchModel.clear();
}

void QCsv::sync() {
    // 简单实现：重新保存
    save();
}

void QCsv::setSeparator(char sep) {
    if (sep != separator) {
        separator = sep;
        // 如果已经加载了数据，需要重新解析
        if (!csvModel.isEmpty()) {
            qWarning() << "Separator changed after loading data. Call load() again.";
        }
    }
}

char QCsv::getSeparator() const {
    return separator;
}