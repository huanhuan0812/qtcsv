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

// ==================== QCsv 实现 ====================

QCsv::QCsv(QObject* parent) : QObject(parent) {}

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
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Could not open file: " + filePath.toStdString());
    }
    
    csvModel.clear();
    searchModel.clear();
    
    const qint64 CHUNK_SIZE = 1024 * 1024; // 1MB
    QByteArray buffer;
    buffer.reserve(CHUNK_SIZE);
    
    CsvParser parser(csvModel, searchModel, separator);
    
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
            removeFromSearch(oldValue, key);
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
    auto [begin, end] = searchModel.equal_range(key);
for (auto it = begin; it != end; ) {
    if (it.value() == key) {
        it = searchModel.erase(it);
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