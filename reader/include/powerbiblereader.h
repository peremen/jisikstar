#ifndef POWERBIBLEREADER_H
#define POWERBIBLEREADER_H

#include <QVector>
#include <QFile>
#include "defs.h"

// book, chapter, verse
struct bible_book_data {
    int book_id;
    QString book_desc;
    QVector<int> chapter_verse;
};


class PowerBibleReader : public QObject {
    Q_OBJECT
private:
    QString filename;

    QVector<bible_book_data> book_data;
    QVector<quint32> index_pos, index_id;
    Snotra::database_args args;
    QString get_chapter(const int, const int, QFile&) const;
    Snotra::dictionary_header_bible header_data;

public:
    explicit PowerBibleReader(QObject *parent = 0);
    PowerBibleReader(const QString &filename, Snotra::database_args args, QObject *parent = 0);
    ~PowerBibleReader();

    bool load(const QString &filename, Snotra::database_args args);
    bool exportToFile(const QString &destination, Snotra::FileType type);

};

#endif // POWERBIBLEMODEL_H
