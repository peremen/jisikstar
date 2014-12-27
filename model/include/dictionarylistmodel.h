#ifndef DICTIONARYLISTMODEL_H
#define DICTIONARYLISTMODEL_H

#include <QAbstractListModel>
#include <QUrl>
#include <QIcon>
#include <QString>
#include <QVector>

struct DictionaryInfo {
    QIcon icon;
    QString name;
    QString fileName;
    int argument;
    int type;
};

class DictionaryListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QUrl path READ getpath WRITE setpath)

public:
    explicit DictionaryListModel(QObject *parent = 0);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QUrl getpath();
    void setpath(QUrl f);

    enum roles {
        FileNameRole = Qt::UserRole + 1,
        ArgumentRole = Qt::UserRole + 2,
        TypeRole = Qt::UserRole + 3
    };
signals:

public slots:

private:
    QUrl path;
    int count;
    QVector<DictionaryInfo> d;
    void update();
};

#endif // DICTIONARYLISTMODEL_H
