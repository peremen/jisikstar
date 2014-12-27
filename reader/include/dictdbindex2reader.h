#ifndef DICTDBINDEX2READER_H
#define DICTDBINDEX2READER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QTextCodec>
#include <QByteArray>
#include <QMap>
#include <QPair>
#include <QHash>
#include "defs.h"

class DictDBIndex2Reader : public QObject {
    Q_OBJECT
private:
    QString filename;
    QTextCodec *codec;
    bool utf16, is_len_10k_uint32;

    Snotra::dictionary_header_index2 header_data;
    QList<Snotra::preindex_data> preindex[2];

    QList<QPair<QString, quint32> > wordTo10KIndex[2];
    QHash<quint32, QPair<quint32, quint32> > tenKHashMap;
    QHash<quint32, QByteArray> compDataIndex;

    void populatePreindex(int preindexId);
    void populate10KIndex();
    void populateDictionaryData();

public:
    explicit DictDBIndex2Reader(QObject *parent = 0);
    DictDBIndex2Reader(const QString &filename, QObject *parent = 0);

    bool load(const QString &filename);
    bool exportToFile(const QString &destination, Snotra::FileType type,
                      int preindexId);

signals:
    void keywordChanged(QString keyword);

public slots:

};

#endif // DICTDBINDEX2READER_H
