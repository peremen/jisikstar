#ifndef POWERBIBLEMODEL_H
#define POWERBIBLEMODEL_H

#include <QAbstractItemModel>
#include <QVector>
#include "defs.h"

// book, chapter, verse
struct bible_book_data {
    int book_id;
    QString book_desc;
    QVector<int> chapter_verse;
};


class PowerBibleModel : public QAbstractItemModel {
    Q_OBJECT
private:
    QString filename;
    bool is_loaded;

    QVector<bible_book_data> book_data;
    QVector<quint32> index_pos, index_id;
    Snotra::database_args args;
    QString get_chapter(const int, const int) const;
    Snotra::dictionary_header_bible header_data;

public:
    explicit PowerBibleModel(QObject *parent = 0);
    PowerBibleModel(const QString &filename, Snotra::database_args args, QObject *parent = 0);
    ~PowerBibleModel();
    bool load(const QString &filename, Snotra::database_args args);
    bool get_load_status();

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

signals:

public slots:

};

#endif // POWERBIBLEMODEL_H
