#include "dictionarylistmodel.h"
#include <QDir>
#include <QUrl>
#include <QDomDocument>
#include <QDebug>

DictionaryListModel::DictionaryListModel(QObject *parent) :
    QAbstractListModel(parent) {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "title";
    roles[FileNameRole] = "fileName";
    roles[ArgumentRole] = "argument";
    roles[TypeRole] = "type";
#if QT_VERSION < 0x050000
    setRoleNames(roles);
#endif
    this->count = 0;
}

int DictionaryListModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return this->count;
}

QVariant DictionaryListModel::data(const QModelIndex &index, int role) const {
    QVariant ret;
    if (!index.isValid()) return ret;
    int idx = index.row();

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        ret = this->d.at(idx).name;
        break;
    case Qt::DecorationRole:
        ret = this->d.at(idx).icon;
        break;
    case FileNameRole:
        ret = this->d.at(idx).fileName;
        break;
    case ArgumentRole:
        ret = this->d.at(idx).argument;
        break;
    case TypeRole:
        ret = this->d.at(idx).type;
        break;
    }

    return ret;
}

QVariant DictionaryListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

QUrl DictionaryListModel::getpath() {
    return this->path;
}

void DictionaryListModel::setpath(QUrl f) {
    if (this->path == f) return;
    if (QDir(f.toLocalFile()).isReadable()) {
        this->path = f;
        update();
    }
}

void DictionaryListModel::update() {
    if (this->count > 0) {
        emit beginRemoveRows(QModelIndex(), 0, this->count - 1);
        this->count = 0;
        this->d.clear();
        emit endRemoveRows();
    }

    QFile f(":/files.xml");
    QString err;
    QDomDocument doc("SnotraFiles");
    if (!f.open(QIODevice::ReadOnly)) return;
    if (!doc.setContent(&f, &err)) {
        f.close();
        return;
    }
    f.close();

    bool file_check = true;

    QDomElement e = doc.documentElement();
    QDomNode n = e.firstChild();
    QString temp_fname;
    while (!n.isNull()) {
        if (n.nodeName() != "Dictionary") {
            n = n.nextSibling();
            continue;
        }
        QDomElement elem = n.toElement();
        if (elem.isNull()) {
            n = n.nextSibling();
            continue;
        }
        DictionaryInfo item;
        item.name = elem.attribute("name");
        file_check = true;

        QDomElement n2 = n.firstChildElement();
        while (!n2.isNull()) {
            if (n2.tagName() == "Icon") {
                item.icon = QIcon(n2.text());
            } else if (n2.tagName() == "File") {
                temp_fname = QString("%1/%2").arg(this->path.toLocalFile(), n2.text());
                file_check &= QFile(temp_fname).exists();
                if (!n2.attribute("main").isNull()) item.fileName = temp_fname;
            } else if (n2.tagName() == "Arg") {
                item.argument = n2.text().toInt();
            } else if (n2.tagName() == "Type") {
                item.type = n2.text().toInt();
            }
            n2 = n2.nextSiblingElement();
        }
        if (file_check) this->d.append(item);

        n = n.nextSiblingElement();
    }

    if (this->d.count()) {
        emit beginInsertRows(QModelIndex(), 0, this->d.count() - 1);
        this->count = this->d.count();
        emit endInsertRows();
    }
}
