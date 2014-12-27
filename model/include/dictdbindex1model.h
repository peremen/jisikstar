#ifndef DICTDBMODEL_H
#define DICTDBMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QTextCodec>
#include "defs.h"

class DictDBIndex1Model : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString keyword READ get_keyword WRITE set_keyword RESET init_keyword NOTIFY keywordChanged)
    Q_PROPERTY(int preindex_count READ get_preindex_count)
private:
    QString filename, keyword;
    int keyword_pos_upper, keyword_pos_lower, keyword_pos_loaded;

    bool is_loaded;
    QList<Snotra::preindex_data> preindex;
    QList<Snotra::dict_data_2> loaded_data;
    QString get_data(const QModelIndex &index) const;
    void clear_all();
    void refresh_index();
    QTextCodec *codec;
    bool utf16;

public:
    Snotra::dictionary_header_index1 header_data;
    explicit DictDBIndex1Model(QObject *parent = 0);
    DictDBIndex1Model(const QString &filename, QObject *parent = 0);
    ~DictDBIndex1Model();
    bool load(const QString &filename);
    bool get_load_status();
    QString get_keyword();
    void set_keyword(const QString &keyword);
    void init_keyword();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);
    int get_preindex_count();

signals:
    void keywordChanged(QString keyword);

public slots:

};

#endif // DICTDBMODEL_H
