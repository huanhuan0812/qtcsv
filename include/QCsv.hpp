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

//key example "A1" like in Excel

class QTCSV_EXPORT QCsv : public QObject {
    Q_OBJECT
public:
    //QCsv(QObject* parent = nullptr);
    QCsv(QString filePath, QObject* parent = nullptr);
    ~QCsv();

    void open(QString filePath);
    void close();
    bool isOpen() const;

    void load();
    void save();
    void saveAs(QString filePath);
    void clear();
    void sync();

    QString getValue(const QString& key) const;
    QList<QString> search(QString index) const;

    void setValue(const QString& key, const QString& value);

    void setSeparator(char sep);
    char getSeparator() const;

private:
    QString filePath;
    QHash<QString, QString> csvModel;
    QMultiMap<QString, QString> searchModel;
    char separator = ',';
    bool opened = false;
    QString numberToColumnRow(int number) const;
};