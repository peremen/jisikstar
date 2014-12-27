#ifndef DICTDBINDEX1READER_H
#define DICTDBINDEX1READER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QTextCodec>
#include <QByteArray>
#include <QMap>
#include <QPair>
#include <QHash>
#include "defs.h"

class DictDBIndex1Reader : public QObject {
    Q_OBJECT

private:
    QString filename;
    QTextCodec *codec;
    bool utf16, is_len_10k_uint32;

    Snotra::dictionary_header_index1 header_data;
    QList<Snotra::preindex_data> preindex;
    QList<QPair<QString, quint32> > wordTo10KIndex;
    QHash<quint32, QPair<quint32, quint32> > tenKHashMap;
    QHash<quint32, QByteArray> compDataIndex;

    void populatePreindex();
    void populate10KIndex();
    void populateDictionaryData();

public:
    explicit DictDBIndex1Reader(QObject *parent = 0);
    DictDBIndex1Reader(const QString &filename, QObject *parent = 0);

    bool load(const QString &filename);
    bool exportToFile(const QString &destination, Snotra::FileType type);
};

#endif // DICTDBINDEX1READER_H
