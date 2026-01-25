#include "QCsv.hpp"
#include <fstream>
#include <QDebug>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <QFile>
#include <QSaveFile>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <vector>
#include <memory>

QCsv::QCsv(QString filePath, QObject* parent)
    : QObject(parent), filePath(filePath), opened(false) {
    
    open(filePath);
}

QCsv::~QCsv() {
    try {
        closeStream();
        if(!opened&& !filePath.isEmpty()) {
            //save();
        }
    } catch (const std::exception& e) {
        qWarning() << "Error in QCsv destructor: " << e.what();
    } catch (...) {
        qWarning() << "Unknown error in QCsv destructor";
    }
        
}

QCsv::QCsv(QCsv&& other) noexcept
    : QObject(other.parent()),
      filePath(std::move(other.filePath)),
      csvModel(std::move(other.csvModel)),
      searchModel(std::move(other.searchModel)),
      separator(other.separator),
      opened(other.opened),
      fileStream(std::move(other.fileStream)),
      currentRow(other.currentRow),
      currentCol(other.currentCol),
      currentCell(std::move(other.currentCell)),
      state(other.state),
      pendingCR(other.pendingCR),
      atEnd(other.atEnd) {
    
    other.filePath.clear();
    other.opened = false;

    other.deleteLater();
}

void QCsv::open(QString filePath) {
    if(filePath.isEmpty()) {
        throw std::runtime_error("File path cannot be empty");
    }
    this->filePath = filePath;
    QFile file(filePath);
    if (!file.exists()) {
        // 如果文件不存在，创建一个空文件
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("Could not create file: " + filePath.toStdString());
        }
        file.close();
    }
    opened = true;
}

void QCsv::close() {
    if (!isOpen()) {
        throw std::runtime_error("File is not open");
    }
    //save();
    filePath.clear();
    opened = false;
    clear();
    closeStream();
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
        case STATE_END_OF_CELL:
            // 从单元格结束状态恢复到正常状态
            if (ch == separator) {
                endCell();
            } else if (ch == '\n') {
                endRow();
                state = STATE_END_OF_ROW;
            } else if (ch == '\r') {
                pendingCR = true;
                endRow();
                state = STATE_END_OF_ROW;
            } else {
                currentCell.append(ch);
                state = STATE_NORMAL;
            }
            break;
        case STATE_END_OF_ROW:
            // 从行结束状态恢复到正常状态
            if (ch == separator) {
                endCell();
                state = STATE_END_OF_CELL;
            } else if (ch == '\n') {
                endRow();
            } else if (ch == '\r') {
                pendingCR = true;
                endRow();
            } else if (ch == '"') {
                currentCell.append(ch);
                state = STATE_IN_QUOTES;
            } else {
                currentCell.append(ch);
                state = STATE_NORMAL;
            }
            break;
        default:
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
        CsvUtils::numberToColumnRow(currentCol)).arg(currentRow + 1);
    
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
    if (newFilePath.isEmpty()) {
        throw std::runtime_error("File path cannot be empty for saving");
    }

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
            col = col * 26 + (colStr[i].toUpper().toLatin1() - 'A' + 1);
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
            QString colStr = CsvUtils::numberToColumnRow(col - 1);
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

void QCsv::atomicSave() {
    atomicSaveAs(filePath);
}

void QCsv::atomicSaveAs(QString filePath) {
    if(filePath.isEmpty()) {
        throw std::runtime_error("File path cannot be empty for atomic save");
    }
    QSaveFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Could not open file for atomic saving");
    }

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
            col = col * 26 + (colStr[i].toUpper().toLatin1() - 'A' + 1);
        }
        maxCol = std::max(maxCol, col);
    }

    // 写入CSV文件
    QTextStream out(&saveFile);
    for (int row = 1; row <= maxRow; ++row) {
        for (int col = 1; col <= maxCol; ++col) {
            // 转换列号回字母
            QString colStr = CsvUtils::numberToColumnRow(col - 1);
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
                out << "\"" << value << "\"";
            } else {
                out << value;
            }

            if (col < maxCol) {
                out << separator;
            }
        }
        out << "\n";
    }
    if (!saveFile.commit()) {
        throw std::runtime_error("Could not commit atomic save");
    }
}

void QCsv::clear() {
    csvModel.clear();
    searchModel.clear();
}

void QCsv::sync() {
    QFuture<void> future = QtConcurrent::run([this]() {
        try {
            save();
        } catch (const std::exception& e) {
            qWarning() << "Error during sync:" << e.what();
        }
    });
    Q_UNUSED(future);
}

void QCsv::finalize() {
    save();
    close();
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

QCsv& operator>>(QCsv& csv, QString& value) {
    if (!csv.fileStream) {
        csv.openStream();
    }
    
    if (csv.atEnd) {
        value.clear();
        return csv;
    }
    
    bool success = csv.readNextCell(value);
    if (!success) {
        csv.atEnd = true;
        value.clear();
    }
    
    return csv;
}

void QCsv::openStream() {
    if (fileStream) {
        return;
    }
    
    try {
        fileStream = std::make_unique<std::ifstream>(
            filePath.toStdString(), 
            std::ios::binary
        );
            
        if (!fileStream->is_open()) {
            throw std::runtime_error("Could not open file stream: " + filePath.toStdString());
        }
            
        // 分配缓冲区
        buffer.resize(BUFFER_SIZE);
        
        // 设置C++流缓冲区（可选，进一步提升性能）
        fileStream->rdbuf()->pubsetbuf(nullptr, 0); // 禁用内部缓冲，使用我们的缓冲
        
    } catch(...) {
        fileStream.reset();
        throw;
    }
    
    state = CsvParser::STATE_NORMAL;
    pendingCR = false;
    atEnd = false;
    currentRow = 0;
    currentCol = 0;
    currentCell.clear();
}

void QCsv::closeStream() {
    if (fileStream) {
        fileStream->close();
        fileStream.reset();
    }
}

bool QCsv::getNextChar(char& ch) {
    if (bufferPos >= bufferSize) {
        if (fileStream->eof()) return false;
        
        fileStream->read(buffer.data(), BUFFER_SIZE);
        bufferSize = fileStream->gcount();
        bufferPos = 0;
        
        if (bufferSize == 0) return false;
    }
    
    ch = buffer[bufferPos++];
    return true;
}

void QCsv::apppendToCurrentCell(char ch) {
    currentCell.append(ch);
    if(currentCell.size() > currentCell.capacity() -10) {
        currentCell.reserve(currentCell.size() + 64);
    }
}

bool QCsv::readNextCell(QString& result){
    if (!fileStream || !fileStream->is_open()) {
        return false;
    }
    
    result.clear();
    currentCell.clear();
    currentCell.reserve(64); // 预分配一些空间以提高性能
    
    char ch;
    while (getNextChar(ch)) {
        switch (state) {
            case CsvParser::STATE_NORMAL:
                if (ch == '"') {
                    state = CsvParser::STATE_IN_QUOTES;
                } else if (ch == separator) {
                    endCell();
                    result = currentCell;
                    return true;
                } else if (ch == '\n') {
                    if (pendingCR) {
                        pendingCR = false;
                    } else {
                        endCell();
                        endRow();
                        result = currentCell;
                        return true;
                    }
                } else if (ch == '\r') {
                    pendingCR = true;
                    endCell();
                    endRow();
                    result = currentCell;
                    return true;
                } else {
                    apppendToCurrentCell(ch);
                }
                break;
                
            case CsvParser::STATE_IN_QUOTES:
                if (ch == '"') {
                    state = CsvParser::STATE_QUOTE_IN_QUOTES;
                } else {
                    apppendToCurrentCell(ch);
                }
                break;
                
            case CsvParser::STATE_QUOTE_IN_QUOTES:
                if (ch == '"') {
                    currentCell.append('"');
                    state = CsvParser::STATE_IN_QUOTES;
                } else if (ch == separator) {
                    endCell();
                    result = currentCell;
                    return true;
                } else if (ch == '\n') {
                    endCell();
                    endRow();
                    result = currentCell;
                    return true;
                } else if (ch == '\r') {
                    pendingCR = true;
                    endCell();
                    endRow();
                    result = currentCell;
                    return true;
                } else {
                    apppendToCurrentCell(ch);
                    state = CsvParser::STATE_NORMAL;
                }
                break;
            default:
                break;
        }
    }
    
    // 处理文件结束
    if (!currentCell.isEmpty() || state != CsvParser::STATE_NORMAL) {
        QCsv::endCell();
        result = currentCell;
        return true;
    }
    
    return false; // 没有更多数据
}

inline void QCsv::endCell() const {
    currentCol++;
}

inline void QCsv::endRow() const {
    currentRow++;
    currentCol = 0;
}

void QCsv::resetStream() {
    closeStream();
    currentRow = 0;
    currentCol = 0;
    currentCell.clear();
    state = CsvParser::STATE_NORMAL;
    pendingCR = false;
    atEnd = false;
}

QCsv& QCsv::operator=(QCsv&& other) noexcept {
    if (this != &other) {
        this->setParent(other.parent());
        filePath = std::move(other.filePath);
        csvModel = std::move(other.csvModel);
        searchModel = std::move(other.searchModel);
        separator = other.separator;
        opened = other.opened;
        fileStream = std::move(other.fileStream);
        currentRow = other.currentRow;
        currentCol = other.currentCol;
        currentCell = std::move(other.currentCell);
        state = other.state;
        pendingCR = other.pendingCR;
        atEnd = other.atEnd;

        other.filePath.clear();
        other.opened = false;
    }
    other.deleteLater();
    return *this;
}

#if EXPERIMENTAL_FUNC

bool QCsv::seekToCell(int targetRow, int targetCol) {
    if (!fileStream || !fileStream->is_open()) {
        return false;
    }
    
    if(targetRow < 0 || targetCol < 0) {
        return false;
    }
    if (targetRow < currentRow || (targetRow == currentRow && targetCol < currentCol)) resetStream();
    
    QString cellValue;

    while (currentRow < targetRow) {
        currentCol = 0;
        while (currentCol <= targetCol) {
            if (!readNextCell(cellValue)) {
                return false; // 到达文件末尾
            }
            currentCol++;
        }
        currentRow++;
    }
    
    return true;
}
#endif