#ifndef DICTDBINDEX2MODEL_H
#define DICTDBINDEX2MODEL_H

#include <QAbstractListModel>
#include <QList>
#include "defs.h"

class DictDBIndex2Model : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString keyword READ get_keyword WRITE set_keyword RESET init_keyword NOTIFY keywordChanged)
    Q_PROPERTY(int preindex_count READ get_preindex_count)
    Q_PROPERTY(int preindex READ get_current_preindex WRITE set_current_preindex)
private:
    QString filename, keyword;
    int keyword_pos_upper, keyword_pos_lower, keyword_pos_loaded;
    int preindex_id;
    bool is_loaded;
    QList<Snotra::preindex_data> preindex[2];
    QList<Snotra::dict_data_2> loaded_data;
    QTextCodec *codec;

    QString get_data(const QModelIndex &index) const;
    void clear_all();
    void refresh_index();

public:
    Snotra::dictionary_header_index2 header_data;
    explicit DictDBIndex2Model(QObject *parent = 0);
    DictDBIndex2Model(const QString &filename, QObject *parent = 0);
    ~DictDBIndex2Model();
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
    int get_current_preindex();
    void set_current_preindex(int preindex);

signals:
    void keywordChanged(QString keyword);

public slots:

};

#endif // DICTDBINDEX2MODEL_H
