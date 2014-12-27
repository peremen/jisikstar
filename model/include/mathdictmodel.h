#ifndef MATHDICTMODEL_H
#define MATHDICTMODEL_H

#include <QAbstractItemModel>
#include <defs.h>

class MathDictModel : public QAbstractItemModel {
    Q_OBJECT
private:
    QString filename;

    bool is_loaded;
    struct Snotra::dict_data *root_data;
    void clear_root_data();
    QPixmap get_data(const QModelIndex &index) const;
public:
    Snotra::dictionary_header_math header_data;
    explicit MathDictModel(QObject *parent = 0);
    MathDictModel(const QString &filename, QObject *parent = 0);
    ~MathDictModel();
    bool load(const QString &filename);
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

#endif // MATHDICTMODEL_H
