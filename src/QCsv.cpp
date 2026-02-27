#include "QCsv.hpp"
#include <fstream>
#include <QDebug>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <QFile>
#include <QSaveFile>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QTextStream>
#include <QStringBuilder>

// ==================== CsvParser 实现 ====================

CsvParser::CsvParser(QHash<QString, QString>& csvModel, 
                     QMultiMap<QString, QString>& searchModel,
                     char separator)
    : csvModel(csvModel), searchModel(searchModel), separator(separator) {}

void CsvParser::resetStatistics() {
    stats = Statistics{};
}

void CsvParser::processChar(char ch) {
    switch (state) {
        case STATE_NORMAL:
            if (ch == '"') {
                state = STATE_IN_QUOTES;
            } else if (ch == separator) {
                endCell();
            } else if (ch == '\n') {
                if (!pendingCR) {
                    endCell();
                    endRow();
                }
                pendingCR = false;
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
                currentCell.append(ch);
                state = STATE_NORMAL;
            }
            break;
            
        case STATE_END_OF_CELL:
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
    }
}

void CsvParser::endCell() {
    stats.maxCol = std::max(stats.maxCol, currentCol);
    
    if (!currentCell.isEmpty()) {
        insertCell();
        stats.totalCells++;
    } else {
        stats.emptyCells++;
    }
    
    currentCell.clear();
    currentCol++;
}

void CsvParser::endRow() {
    if (pendingCR) {
        if (!currentCell.isEmpty()) {
            endCell();
        }
        pendingCR = false;
    }
    
    stats.maxRow = std::max(stats.maxRow, currentRow);
    
    currentRow++;
    currentCol = 0;
}

void CsvParser::insertCell() {
    QString key = CsvUtils::numberToColumnRow(currentCol) % QString::number(currentRow + 1);
    
    stats.maxRow = std::max(stats.maxRow, currentRow + 1);
    stats.maxCol = std::max(stats.maxCol, currentCol + 1);

    if (!currentCell.isEmpty()) {
        csvModel.insert(key, currentCell);
        searchModel.insert(currentCell, key);
    }
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
        qWarning() << "CSV file ended inside quoted field";
    }
    
    if (!currentCell.isEmpty() || state != STATE_NORMAL) {
        endCell();
    }
    
    if (currentCol > 0) {
        endRow();
    }
}

// ==================== Utf8CsvParser 实现 ====================

Utf8CsvParser::Utf8CsvParser(QHash<QString, QString>& csvModel, 
                             QMultiMap<QString, QString>& searchModel,
                             char separator)
    : csvModel(csvModel), searchModel(searchModel), separator(separator) {
        currentCell.reserve(256);  // 预分配空间
        utf8Buffer.reserve(8);  
    }

void Utf8CsvParser::resetStatistics() {
    stats = Statistics{};
}

void Utf8CsvParser::processChar(QChar ch) {
    // 原有 CsvParser 的 processChar 逻辑，但处理 QChar
    char ascii = ch.toLatin1();
    
    switch (state) {
        case STATE_NORMAL:
            if (ch == '"') {
                state = STATE_IN_QUOTES;
            } else if (ascii == separator) {
                endCell();
            } else if (ch == '\n') {
                if (!pendingCR) {
                    endCell();
                    endRow();
                }
                pendingCR = false;
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
                currentCell.append('"');
                state = STATE_IN_QUOTES;
            } else if (ascii == separator) {
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
                currentCell.append(ch);
                state = STATE_NORMAL;
            }
            break;
            
        case STATE_END_OF_CELL:
            if (ascii == separator) {
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
            if (ascii == separator) {
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
    }
}

void Utf8CsvParser::flushUtf8Char() {
    if (!utf8Buffer.isEmpty()) {
        QString str = QString::fromUtf8(utf8Buffer);
        for (QChar ch : str) {
            processChar(ch);
        }
        utf8Buffer.clear();
    }
}

void Utf8CsvParser::processUtf8Char(const char*& ptr, const char* end) {
    unsigned char c = *ptr++;
    
    if (c < 0x80) {
        // ASCII 字符，直接处理
        flushUtf8Char();  // 先刷新可能存在的UTF-8缓冲区
        processChar(QChar::fromLatin1(static_cast<char>(c)));
        return;
    }
    
    // UTF-8 多字节字符开始
    utf8Buffer.append(static_cast<char>(c));
    
    // 确定UTF-8字符的字节数
    int bytesNeeded = 1;
    if ((c & 0xE0) == 0xC0) {
        bytesNeeded = 2;  // 2字节字符: 110xxxxx 10xxxxxx
    } else if ((c & 0xF0) == 0xE0) {
        bytesNeeded = 3;  // 3字节字符: 1110xxxx 10xxxxxx 10xxxxxx
    } else if ((c & 0xF8) == 0xF0) {
        bytesNeeded = 4;  // 4字节字符: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    } else {
        // 无效的UTF-8起始字节，当作ASCII处理
        flushUtf8Char();
        processChar(QChar::fromLatin1(static_cast<char>(c)));
        return;
    }
    
    // 读取剩余的字节
    for (int i = 1; i < bytesNeeded && ptr < end; ++i) {
        utf8Buffer.append(*ptr++);
    }
    
    // 如果收集了完整的UTF-8字符，转换为QChar并处理
    if (utf8Buffer.size() == bytesNeeded) {
        flushUtf8Char();
    }
    // 否则等待更多数据
}

void Utf8CsvParser::parse(const char* data, size_t size, bool isFinal) {
    const char* ptr = data;
    const char* end = data + size;
    
    while (ptr < end) {
        processUtf8Char(ptr, end);
    }
    
    if (isFinal) {
        // 处理最后一个可能的UTF-8字符
        flushUtf8Char();
        finalize();
    }
}

void Utf8CsvParser::finalize() {
    if (state == STATE_IN_QUOTES) {
        qWarning() << "CSV file ended inside quoted field";
    }
    
    flushUtf8Char();
    
    if (!currentCell.isEmpty() || state != STATE_NORMAL) {
        endCell();
    }
    
    if (currentCol > 0) {
        endRow();
    }
}

void Utf8CsvParser::endCell() {
    stats.maxCol = std::max(stats.maxCol, currentCol);
    
    if (!currentCell.isEmpty()) {
        insertCell();
        stats.totalCells++;
    } else {
        stats.emptyCells++;
    }
    
    currentCell.clear();
    currentCol++;
}

void Utf8CsvParser::endRow() {
    if (pendingCR) {
        if (!currentCell.isEmpty()) {
            endCell();
        }
        pendingCR = false;
    }
    
    stats.maxRow = std::max(stats.maxRow, currentRow);
    
    currentRow++;
    currentCol = 0;
}

void Utf8CsvParser::insertCell() {
    QString key = CsvUtils::numberToColumnRow(currentCol) % QString::number(currentRow + 1);
    
    stats.maxRow = std::max(stats.maxRow, currentRow + 1);
    stats.maxCol = std::max(stats.maxCol, currentCol + 1);

    if (!currentCell.isEmpty()) {
        csvModel.insert(key, currentCell);
        searchModel.insert(currentCell, key);
    }
}

// ==================== QCsv 实现 ====================

QCsv::QCsv(const QString& filePath, QObject* parent)
    : QObject(parent), filePath(filePath) {
    try {
        open(filePath);
    } catch (const std::exception& e) {
        emit error(tr("Failed to open file: %1").arg(e.what()));
    }
}

QCsv::~QCsv() {
    try {
        closeStream();
    } catch (const std::exception& e) {
        qWarning() << "Error in QCsv destructor:" << e.what();
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
    
    // 完全重置原对象的状态
    other.opened = false;
    other.currentRow = 0;
    other.currentCol = 0;
    other.currentCell.clear();
    other.state = CsvParser::STATE_NORMAL;
    other.pendingCR = false;
    other.atEnd = true;  // 设置为已结束
}

QCsv& QCsv::operator=(QCsv&& other) noexcept {
    if (this != &other) {
        setParent(other.parent());
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
        other.currentRow = 0;
        other.currentCol = 0;
        other.currentCell.clear();
        other.state = CsvParser::STATE_NORMAL;
        other.pendingCR = false;
        other.atEnd = true;
    }
    return *this;
}

void QCsv::open(const QString& filePath) {
    if (filePath.isEmpty()) {
        throw std::runtime_error("File path cannot be empty");
    }
    
    this->filePath = filePath;
    QFile file(filePath);
    
    if (!file.exists()) {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("Could not create file: " + filePath.toStdString());
        }
        file.close();
    }
    
    opened = true;
    emit fileOpened(filePath);
}

void QCsv::close() {
    if (!isOpen()) {
        throw std::runtime_error("File is not open");
    }
    opened = false;
    clear();
    closeStream();
    emit fileClosed();
}

bool QCsv::isOpen() const {
    return opened && !filePath.isEmpty();
}

void QCsv::load() {
    if (!opened || filePath.isEmpty()) {
        throw std::runtime_error("File not opened");
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {  // 注意：不要加 Text 标志
        throw std::runtime_error("Could not open file: " + filePath.toStdString());
    }
    
    csvModel.clear();
    searchModel.clear();
    
    const qint64 CHUNK_SIZE = 1024 * 1024; // 1MB
    QByteArray buffer;
    buffer.reserve(CHUNK_SIZE);
    
    // 使用新的 UTF-8 感知解析器
    Utf8CsvParser parser(csvModel, searchModel, separator);
    
    while (!file.atEnd()) {
        buffer = file.read(CHUNK_SIZE);
        if (!buffer.isEmpty()) {
            parser.parse(buffer.constData(), buffer.size(), false);
        }
    }
    
    parser.finalize();
    file.close();
    
    // 更新最大行列
    auto stats = parser.getStatistics();
    maxRow = std::max(1, stats.maxRow);
    maxCol = std::max(1, stats.maxCol);
    
    qDebug() << "Loaded" << csvModel.size() << "cells from CSV";
}

bool QCsv::save() {
    if (!isOpen()) {
        emit error("No file opened for saving");
        return false;
    }
    return saveAs(filePath);
}

bool QCsv::saveAs(const QString& newFilePath) {
    if (newFilePath.isEmpty()) {
        emit error("File path cannot be empty for saving");
        return false;
    }
    
    QFile file(newFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit error("Could not open file for writing: " + newFilePath);
        return false;
    }
    
    QTextStream out(&file);

    out.setEncoding(QStringConverter::Utf8);

    bool success = writeToStream(out);
    file.close();
    
    if (success) {
        emit fileSaved(newFilePath);
    }
    
    return success;
}

bool QCsv::atomicSave() {
    return atomicSaveAs(filePath);
}

bool QCsv::atomicSaveAs(const QString& filePath) {
    if (filePath.isEmpty()) {
        emit error("File path cannot be empty for atomic save");
        return false;
    }
    
    QSaveFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit error("Could not open file for atomic saving: " + filePath);
        return false;
    }

    QTextStream out(&saveFile);

    out.setEncoding(QStringConverter::Utf8);

    bool success = writeToStream(out);
    
    if (success && saveFile.commit()) {
        emit fileSaved(filePath);
        return true;
    }
    
    saveFile.cancelWriting();
    emit error("Could not commit atomic save for: " + filePath);
    return false;
}

bool QCsv::writeToStream(QTextStream& out) const {
    try {
        for (int row = 1; row <= maxRow; ++row) {
            for (int col = 1; col <= maxCol; ++col) {
                QString colStr = CsvUtils::numberToColumnRow(col - 1);
                QString key = colStr % QString::number(row);
                QString value = csvModel.value(key);
                
                if (CsvUtils::needsQuotes(value, separator)) {
                    out << '"' << CsvUtils::escapeQuotes(value) << '"';
                } else {
                    out << value;
                }

                if (col < maxCol) {
                    out << separator;
                }
            }
            out << '\n';
        }
        return true;
    } catch (const std::exception& e) {
        qWarning() << "Error writing to stream:" << e.what();
        return false;
    }
}

void QCsv::clear() {
    csvModel.clear();
    searchModel.clear();
    maxRow = 1;
    maxCol = 1;
}

QFuture<bool> QCsv::sync() {
    return QtConcurrent::run([this]() {
        return save();
    });
}

void QCsv::finalize() {
    if (save()) {
        close();
    }
}

QString QCsv::getValue(const QString& key) const {
    return csvModel.value(key);
}

std::optional<QString> QCsv::tryGetValue(const QString& key) const {
    auto it = csvModel.find(key);
    if (it != csvModel.end()) {
        return *it;
    }
    return std::nullopt;
}

void QCsv::setValue(const QString& key, const QString& value) {
    QString oldValue = csvModel.value(key);
    
    if (value.isEmpty()) {
        if (!oldValue.isEmpty()) {
            csvModel.remove(key);
            removeFromSearch(oldValue, key);
        }
    } else {
        if (!oldValue.isEmpty()) {
            if (oldValue != value) {            // ✅ 避免值相同时重复操作
                removeFromSearch(oldValue, key);
            } else {
                // 值相同且非空，无需任何操作，可直接返回（或保留现有逻辑但跳过更新）
                // 注意：如果此处直接返回，需确保 dataChanged 不会被发射
                return;  // 或者跳过后续插入，因为 csvModel 中已有相同键值对
            }
        }
        csvModel.insert(key, value);
        searchModel.insert(value, key);
        updateMaxRowCol(key);
    }
    
    if (oldValue != value) {
        emit dataChanged(key, oldValue, value);
    }
}

void QCsv::setValues(const QHash<QString, QString>& values) {
    for (auto it = values.begin(); it != values.end(); ++it) {
        setValue(it.key(), it.value());
    }
}

void QCsv::removeFromSearch(const QString& value, const QString& key) {
    auto [begin, end] = searchModel.equal_range(value);
    for (auto it = begin; it != end; ) {
        if (it.value() == key) {
            it = searchModel.erase(it);
            break;
        } else {
            ++it;
        }
    }
}

void QCsv::updateMaxRowCol(const QString& key) {
    auto [colPart, rowPart] = CsvUtils::splitKey(key);
    maxRow = std::max(maxRow, rowPart + 1);
    maxCol = std::max(maxCol, CsvUtils::columnRowToNumber(colPart) + 1);
}

QList<QString> QCsv::search(const QString& value) const {
    return searchModel.values(value);
}

QList<QString> QCsv::searchByPrefix(const QString& prefix) const {
    QList<QString> results;
    if (prefix.isEmpty()) return results;
    
    auto it = searchModel.lowerBound(prefix);
    while (it != searchModel.end() && it.key().startsWith(prefix)) {
        results.append(it.value());
        ++it;
    }
    return results;
}

void QCsv::setSeparator(char sep) {
    if (sep != separator) {
        separator = sep;
        if (!csvModel.isEmpty()) {
            qWarning() << "Separator changed after loading data. Call load() again.";
        }
    }
}

// 流式读取操作符
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

// 流式读取辅助方法
void QCsv::openStream() {
    if (fileStream) return;
    
    fileStream = std::make_unique<std::ifstream>(
        filePath.toStdString(), std::ios::binary
    );
        
    if (!fileStream->is_open()) {
        throw std::runtime_error("Could not open file stream: " + filePath.toStdString());
    }
    
    buffer.resize(BUFFER_SIZE);
    fileStream->rdbuf()->pubsetbuf(nullptr, 0);
    
    state = CsvParser::STATE_NORMAL;
    pendingCR = false;
    atEnd = false;
    currentRow = 0;
    currentCol = 0;
    currentCell.clear();
    currentCell.reserve(64);
}

void QCsv::closeStream() {
    if (fileStream) {
        fileStream->close();
        fileStream.reset();
    }
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

void QCsv::appendToCurrentCell(char ch) {
    currentCell.append(ch);
    if (currentCell.size() > currentCell.capacity() - 10) {
        currentCell.reserve(currentCell.size() + 64);
    }
}

bool QCsv::readNextCell(QString& result) {
    if (!fileStream || !fileStream->is_open()) {
        return false;
    }
    
    result.clear();
    currentCell.clear();
    
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
                    if (!pendingCR) {
                        endCell();
                        endRow();
                        result = currentCell;
                        return true;
                    }
                    pendingCR = false;
                } else if (ch == '\r') {
                    pendingCR = true;
                    endCell();
                    endRow();
                    result = currentCell;
                    return true;
                } else {
                    appendToCurrentCell(ch);
                }
                break;
                
            case CsvParser::STATE_IN_QUOTES:
                if (ch == '"') {
                    state = CsvParser::STATE_QUOTE_IN_QUOTES;
                } else {
                    appendToCurrentCell(ch);
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
                    appendToCurrentCell(ch);
                    state = CsvParser::STATE_NORMAL;
                }
                break;
                
            default:
                break;
        }
    }
    
    // 处理文件结束
    if (!currentCell.isEmpty() || state != CsvParser::STATE_NORMAL) {
        endCell();
        result = currentCell;
        return true;
    }
    
    return false;
}

void QCsv::endCell() {
    currentCol++;
}

void QCsv::endRow() {
    currentRow++;
    currentCol = 0;
}

bool QCsv::hasNext() const {
    return !atEnd;
}

void QCsv::enableHeaders(bool enable) {
    if (headersOn == enable) return;  // 避免不必要的操作
    headersOn = enable;
}

// ==================== 列名称 ====================
void QCsv::setHeaderRow(int row) {
    if (row < 1) throw std::invalid_argument("Header row number must be >= 1");
    headerRow = row;
}

void QCsv::setColumnHeader(int col, const QString& header) {
    if (col < 1) throw std::invalid_argument("Column number must be >= 1");
    if(header.isEmpty()) {
        setValue(CsvUtils::numberToColumnRow(col - 1) + QString::number(headerRow), QString()); // 移除标题行的列标题
    } else {
        setValue(CsvUtils::numberToColumnRow(col - 1) + QString::number(headerRow), header); // 设置标题行的列标题
    }
}

void QCsv::setColumnHeaders(const QHash<int, QString>& headers) {
    if (headers.isEmpty()) return;
    
    // 验证所有列号
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        if (it.key() < 1) {
            throw std::invalid_argument("Column number must be >= 1");
        }
    }
    
    // 批量更新
    QHash<QString, QString> batchValues;
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        QString key = CsvUtils::numberToColumnRow(it.key() - 1) + QString::number(headerRow);
        batchValues.insert(key, it.value());
    }
    
    // 使用批量设置方法（如果存在）
    setValues(batchValues);  // 假设有这样的批量方法
}

QString QCsv::getColumnHeader(int col) const {
    return csvModel.value(CsvUtils::numberToColumnRow(col - 1) + QString::number(headerRow));
}

QList<QString> QCsv::getColumnHeaders() const {
    QList<QString> headers;
    for (int col = 1; col <= maxCol; ++col) {
        headers.append(getColumnHeader(col));
    }
    return headers;
}

QStringList QCsv::getColumnHeaderLists() const {
    QStringList headerList;
    for (int col = 1; col <= maxCol; ++col) {
        headerList.append(getColumnHeader(col));
    }
    return headerList;
}

QList<int> QCsv::searchColumnHeader(const QString& header) const {
    QList<int> results;
    if (header.isEmpty()) return results;
    
    // 获取所有匹配的键
    auto keys = searchModel.values(header);
    
    // 遍历所有匹配的键，找到在标题行的那个
    for (const QString& key : keys) {
        auto [colPart, rowPart] = CsvUtils::splitKey(key);
        if (rowPart  == headerRow - 1) {  // rowPart 是0-based，headerRow是1-based
            // qDebug() << "Found header:" << header <<"match at key:" << key << "colPart:" << colPart << "rowPart:" << rowPart;
            results.append(CsvUtils::columnRowToNumber(colPart) + 1);
        }
    }
    std::sort(results.begin(), results.end());
    return results;
}
  

// ==================== 行名称 ====================
void QCsv::setHeaderColumn(int col) {
    if (col < 1) throw std::invalid_argument("Header column number must be >= 1");
    headerCol = col;
}

void QCsv::setRowHeader(int row, const QString& header) {
    if (row < 1) throw std::invalid_argument("Row number must be >= 1");
    if(header.isEmpty()) {
        setValue(CsvUtils::numberToColumnRow(headerCol - 1) + QString::number(row), QString()); // 移除标题列的行标题
    } else {
        setValue(CsvUtils::numberToColumnRow(headerCol - 1) + QString::number(row), header); // 设置标题列的行标题
    }
}

void QCsv::setRowHeaders(const QHash<int, QString>& headers) {
    if (headers.isEmpty()) return;
    
    // 验证所有行号
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        if (it.key() < 1) {
            throw std::invalid_argument("Row number must be >= 1");
        }
    }
    
    // 批量更新
    QHash<QString, QString> batchValues;
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        QString key = CsvUtils::numberToColumnRow(headerCol - 1) + QString::number(it.key());
        batchValues.insert(key, it.value());
    }
    
    // 使用批量设置方法（如果存在）
    setValues(batchValues);  // 假设有这样的批量方法
}

QString QCsv::getRowHeader(int row) const {
    return csvModel.value(CsvUtils::numberToColumnRow(headerCol - 1) + QString::number(row));
}

QList<QString> QCsv::getRowHeaders() const {
    QList<QString> headers;
    for (int row = 1; row <= maxRow; ++row) {
        headers.append(getRowHeader(row));
    }
    return headers;
}

QStringList QCsv::getRowHeaderLists() const {
    QStringList headerList;
    for (int row = 1; row <= maxRow; ++row) {
        headerList.append(getRowHeader(row));
    }
    return headerList;
}

QList<int> QCsv::searchRowHeader(const QString& header) const {
    QList<int> results;
    auto keys = searchModel.values(header);  // ✅ 获取所有匹配项
    QString targetCol = CsvUtils::numberToColumnRow(headerCol - 1);
    
    for (const QString& key : keys) {
        auto [colPart, rowPart] = CsvUtils::splitKey(key);
        if (colPart == targetCol) {
            results.append(rowPart + 1);
        }
    }
    // 排序
    std::sort(results.begin(), results.end());
    return results;
}

//===================== 文件元数据 =====================

QDateTime QCsv::getLastModified() const {
    if(filePath.isEmpty()) {
        throw std::runtime_error("File path is empty, cannot get last modified time");
    }
    QFileInfo fileInfo(filePath);
    return fileInfo.lastModified();
}

qint64 QCsv::getFileSize() const {
    if(filePath.isEmpty()) {
        throw std::runtime_error("File path is empty, cannot get file size");
    }
    QFile file(filePath);
    if (!file.exists()) {
        throw std::runtime_error("File does not exist: " + filePath.toStdString());
    }
    QFileInfo fileInfo(file);
    qint64 size = fileInfo.size();
    return size;
}

QMap<QString, QVariant> QCsv::getAllMetadata() const {
    if(filePath.isEmpty()) {
        throw std::runtime_error("File path is empty, cannot get metadata");
    }
    QFileInfo fileInfo(filePath);
    QMap<QString, QVariant> metadata;
    metadata.insert("lastModified", fileInfo.lastModified());
    metadata.insert("fileSize", fileInfo.size());
    metadata.insert("created", fileInfo.birthTime());
    metadata.insert("isReadable", fileInfo.isReadable());
    metadata.insert("isWritable", fileInfo.isWritable());
    metadata.insert("isExecutable", fileInfo.isExecutable());
    return metadata;
}

// ==================== 类型判断&&转换 ====================

//判断
bool QCsv::isNumeric(const QString& value) const {
    bool ok;
    value.toDouble(&ok);
    return ok;
}

bool QCsv::isDate(const QString& value, const QString& format) const {
    return QDateTime::fromString(value, format).isValid();
}

bool QCsv::isBoolean(const QString& value) const {
    QString lowerValue = value.trimmed().toLower();
    return lowerValue == "true" || lowerValue == "false" || lowerValue == "1" || lowerValue == "0";
}

//转换
std::optional<double> QCsv::toDouble(const QString& value) const {
    bool ok;
    double result = value.toDouble(&ok);
    if (ok) {
        return result;
    }
    return std::nullopt;
}

std::optional<QDate> QCsv::toDate(const QString& value, const QString& format) const {
    QDateTime dateTime = QDateTime::fromString(value, format);
    if (dateTime.isValid()) {
        return dateTime.date();
    }
    return std::nullopt;
}

std::optional<bool> QCsv::toBoolean(const QString& value) const {
    QString lowerValue = value.trimmed().toLower();
    if (lowerValue == "true" || lowerValue == "1") {
        return true;
    } else if (lowerValue == "false" || lowerValue == "0") {
        return false;
    }
    return std::nullopt;
}

#if EXPERIMENTAL_FUNC
bool QCsv::seekToCell(int targetRow, int targetCol) {
    if (!fileStream || !fileStream->is_open()) {
        return false;
    }
    
    if (targetRow < 0 || targetCol < 0) {
        return false;
    }
    
    if (targetRow < currentRow || (targetRow == currentRow && targetCol < currentCol)) {
        resetStream();
    }
    
    QString cellValue;
    while (currentRow <= targetRow) {
        currentCol = 0;
        while (currentCol <= targetCol) {
            if (!readNextCell(cellValue)) {
                return false;
            }
        }
    }
    
    return true;
}
#endif