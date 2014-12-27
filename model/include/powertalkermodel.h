#ifndef POWERTALKERMODEL_H
#define POWERTALKERMODEL_H

#include <QAbstractItemModel>
#include "defs.h"

class PowerTalkerModel : public QAbstractItemModel {
    Q_OBJECT
private:
    QString filename;
    bool is_loaded;
    struct Snotra::dict_data *root_data;

    void clear_root_data();
    Snotra::database_args args;
    QByteArray get_data(const QModelIndex &index) const;
public:
    Snotra::powertalker_header header_data;
    QString aux_filename_1, aux_filename_2;

    explicit PowerTalkerModel(QObject *parent = 0);
    PowerTalkerModel(const QString &filename, Snotra::database_args args, QObject *parent = 0);
    ~PowerTalkerModel();
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
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

signals:

public slots:

};

#endif // POWERTALKERMODEL_H
