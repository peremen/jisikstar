#include "include/powertalkerreader.h"
#include "include/defs.h"
#include "lzmadecoder.h"
#include <QFile>
#include <QByteArray>
#include <QString>
#include <QTextCodec>
#include <QDataStream>
#include <QDebug>

PowerTalkerReader::PowerTalkerReader(QObject *parent) : QAbstractItemModel(parent) {
    root_data = new Snotra::dict_data;
}

PowerTalkerReader::PowerTalkerReader(const QString &filename, Snotra::database_args args, QObject *parent): QAbstractItemModel(parent) {
    root_data = new Snotra::dict_data;
    this->is_loaded = false;
    this->load(filename, args);
}

PowerTalkerReader::~PowerTalkerReader() {
    delete root_data;
}


bool PowerTalkerReader::load(const QString &filename, Snotra::database_args args) {
    if (this->is_loaded) delete root_data;
    QFile f(filename);
    if (!f.exists()) return false;
    if (!f.open(QIODevice::ReadOnly)) return false;
    this->filename = filename;
    this->args = args;

    QDataStream s(&f);
    s.setByteOrder(QDataStream::LittleEndian);
    quint8 header_len;
    s >> header_len;
    QByteArray header;
    header.resize(header_len);
    s.readRawData(header.data(), header_len);

    QDataStream t(header);
    t.setByteOrder(QDataStream::BigEndian);
    t >> this->header_data.dictionary_info_len;
    this->header_data.dictionary_info.resize(this->header_data.dictionary_info_len);
    t.readRawData(this->header_data.dictionary_info.data(), this->header_data.dictionary_info_len);
    t.setByteOrder(QDataStream::LittleEndian);
    t >> this->header_data.record_header_len;
    t >> this->header_data.unk_1 >> this->header_data.unk_2 >> this->header_data.unk_3;
    for (int i = 0; i < 4; i++) {
        t >> this->header_data.lengths[i].orig >> this->header_data.lengths[i].tran;
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            t >> this->header_data.index[i][j].lang >> this->header_data.index[i][j].pos >> this->header_data.index[i][j].len;
        }
    }
    QString base_path = this->filename.mid(0, this->filename.lastIndexOf("/"));
    int idx;
    switch (args) {
    case Snotra::english:
        this->aux_filename_1 = base_path + "/EnglishNativePronWordList.dat";
        this->aux_filename_2 = base_path + "/EnglishNativePronData_000.dat";
        idx = 0;
        break;
    case Snotra::chinese:
        this->aux_filename_1 = base_path + "/ChineseNativePronWordList.dat";
        this->aux_filename_2 = base_path + "/ChineseNativePronData_000.dat";
        idx = 2;
        break;
    case Snotra::japanese:
        this->aux_filename_1 = base_path + "/JapaneseNativePronWordList.dat";
        this->aux_filename_2 = base_path + "/JapaneseNativePronData_000.dat";
        idx = 3;
        break;
    default:
        return false;
    }
    this->clear_root_data();

    f.seek(this->header_data.index[0][idx].pos + 10);
    QByteArray compressed_index = f.read(this->header_data.index[0][idx].len - 10), decompressed_index;
    LZMADecoder::decodeLZMA(compressed_index, decompressed_index);

    QTextCodec *codec = QTextCodec::codecForName("UTF-16LE");
    QString title_original, title_korean;
    qint32 data_id, child_id = 0;
    quint16 trailing_bytes;
    int pos = 0;
    struct Snotra::dict_data *newlevel;

    int i = 0;
    while (true) {
        newlevel = new Snotra::dict_data;
        data_id = *(quint32 *)(decompressed_index.constData() + pos);
        pos += 4;
        title_original = codec->toUnicode(decompressed_index.mid(pos, this->header_data.lengths[idx].orig * 2));
        title_original.remove(QChar('\0'));
        pos += (this->header_data.lengths[idx].orig * 2);
        title_korean = codec->toUnicode(decompressed_index.mid(pos, this->header_data.lengths[idx].tran * 2));
        title_korean.remove(QChar('\0'));
        pos += (this->header_data.lengths[idx].tran * 2);
        trailing_bytes = *(quint16 *)(decompressed_index.constData() + pos);
        pos += 2;
        for (int j = 0; j < trailing_bytes; j++) {
            child_id = *(quint32 *)(decompressed_index.constData() + pos);
            if (j == 0) newlevel->lower = child_id;

            pos += 4;
        }
        newlevel->upper = child_id;

        newlevel->row = i++;
        newlevel->desc = title_original;
        newlevel->leaf = false;
        newlevel->lower_loaded = true;
        newlevel->data_tag = QString("mc%1").arg(data_id, 5, 10, QLatin1Char('0'));
        newlevel->lower_len = 0;
        newlevel->parent = this->root_data;
        this->root_data->lower_data.append(newlevel);

        if (pos >= decompressed_index.size() - 2) break;
    }
    this->root_data->lower_len = i;
    this->root_data->lower_loaded = true;

    f.seek(this->header_data.index[1][idx].pos + 10);
    compressed_index = f.read(this->header_data.index[1][idx].len - 10);
    LZMADecoder::decodeLZMA(compressed_index, decompressed_index);
    struct Snotra::dict_data *newlevel2;

    int row = 0, parent = 0, child_count = child_id;
    pos = 0;
    for (i = 0; i < child_count; i++) {
        newlevel2 = new Snotra::dict_data;
        data_id = *(quint32 *)(decompressed_index.constData() + pos);
        pos += 4;
        title_original = codec->toUnicode(decompressed_index.mid(pos, this->header_data.lengths[idx].orig * 2));
        title_original.remove(QChar('\0'));
        pos += (this->header_data.lengths[idx].orig * 2);
        title_korean = codec->toUnicode(decompressed_index.mid(pos, this->header_data.lengths[idx].tran * 2));
        title_korean.remove(QChar('\0'));
        pos += (this->header_data.lengths[idx].tran * 2);
        trailing_bytes = *(quint16 *)(decompressed_index.constData() + pos);
        pos += 2;
        for (int j = 0; j < trailing_bytes; j++) {
            child_id = *(quint32 *)(decompressed_index.constData() + pos);
            if (j == 0) newlevel2->lower = child_id;
            pos += 4;
        }
        newlevel2->upper = child_id;
        newlevel2->desc = title_original;

        if (data_id > this->root_data->lower_data.at(parent)->upper) {
            row = 0;
            parent++;
        }
        newlevel2->row = row++;
        newlevel2->parent = this->root_data->lower_data.at(parent);
        this->root_data->lower_data.at(parent)->lower_data.append(newlevel2);
        this->root_data->lower_data.at(parent)->lower_len++;
        newlevel2->data_tag = QString("sc%1").arg(data_id, 5, 10, QLatin1Char('0'));
        newlevel2->lower_len = 0;

        newlevel2->leaf = false;
        newlevel2->lower_loaded = true;
    }

    child_count = child_id; pos = 0; row = 0; parent = 0;
    unsigned int parent_2 = 0;
    f.seek(this->header_data.index[2][idx].pos + 10);
    compressed_index = f.read(this->header_data.index[2][idx].len - 10);
    LZMADecoder::decodeLZMA(compressed_index, decompressed_index);
    struct Snotra::dict_data *newlevel3;

    for (i = 0; i < child_count; i++) {
        data_id = *(quint32 *)(decompressed_index.constData() + pos);
        pos += 4;
        title_original = codec->toUnicode(decompressed_index.mid(pos, this->header_data.lengths[idx].orig * 2));
        title_original.remove(QChar('\0'));
        pos += (this->header_data.lengths[idx].orig * 2);
        title_korean = codec->toUnicode(decompressed_index.mid(pos, this->header_data.lengths[idx].tran * 2));
        title_korean.remove(QChar('\0'));
        pos += (this->header_data.lengths[idx].tran * 2);
        if (title_original.isEmpty()) continue;
        newlevel3 = new Snotra::dict_data;
        newlevel3->desc = title_original;
        if (data_id > this->root_data->lower_data.at(parent)->lower_data.at(parent_2)->upper) {
            row = 0;
            parent_2++;
            if (parent_2 >= this->root_data->lower_data.at(parent)->lower_len) {
                parent_2 = 0;
                parent++;
            }
        }
        newlevel3->row = row++;
        newlevel3->parent = this->root_data->lower_data.at(parent)->lower_data.at(parent_2);
        this->root_data->lower_data.at(parent)->lower_data.at(parent_2)->lower_data.append(newlevel3);
        this->root_data->lower_data.at(parent)->lower_data.at(parent_2)->lower_len++;
        newlevel3->data_tag = QString("tx%1").arg(data_id, 5, 10, QLatin1Char('0'));
        newlevel3->leaf = true;
        newlevel3->lower_loaded = true;
    }
    f.close();
    this->is_loaded = true;
    return true;
}

bool PowerTalkerReader::get_load_status() {
    return this->is_loaded;
}

void PowerTalkerReader::clear_root_data() {
    this->root_data->row = 0;
    this->root_data->lower_data.clear();
    this->root_data->desc.clear();
    this->root_data->lower_len = 0;
    this->root_data->lower_loaded = false;
    this->root_data->lower_pos = 0;
    this->root_data->parent = this->root_data;
    this->root_data->leaf = false;
}

QModelIndex PowerTalkerReader::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();
    Snotra::dict_data *parent_data;
    if (!parent.isValid()) {
        parent_data = this->root_data;
    } else {
        parent_data = static_cast<Snotra::dict_data*>(parent.internalPointer());
    }
    if (!parent_data->lower_loaded) return QModelIndex();
    if ((quint64)row > parent_data->lower_len) return QModelIndex();

    Snotra::dict_data *child_data = parent_data->lower_data[row];
    if (child_data) {
        return createIndex(row, column, (void *)child_data);
    } else {
        return QModelIndex();
    }
}

QModelIndex PowerTalkerReader::parent(const QModelIndex &child) const {
    if (!child.isValid()) return QModelIndex();
    struct Snotra::dict_data *child_data = static_cast<Snotra::dict_data *>(child.internalPointer());
    struct Snotra::dict_data *parent_data = child_data->parent;
    if (parent_data == this->root_data) return QModelIndex();
    else {
        return createIndex(parent_data->row, 0, (void*) parent_data);
    }
}

int PowerTalkerReader::rowCount(const QModelIndex &parent) const {
    Snotra::dict_data *parent_data;
    if (parent.column() > 0) return 0;

    if (!parent.isValid()) parent_data = this->root_data;
    else parent_data = static_cast<Snotra::dict_data*>(parent.internalPointer());
    return parent_data->lower_len;
}

int PowerTalkerReader::columnCount(const QModelIndex &) const {
    return 1;
}

bool PowerTalkerReader::hasChildren(const QModelIndex &parent) const {
    if (!parent.isValid()) return true;
    Snotra::dict_data *parent_data = static_cast<Snotra::dict_data*>(parent.internalPointer());
    return !parent_data->leaf;

}

QVariant PowerTalkerReader::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    Snotra::dict_data *itemdata = static_cast<Snotra::dict_data*>(index.internalPointer());
    if (!itemdata) return QVariant();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return itemdata->desc;
    case Snotra::ItemPositionRole:
        return itemdata->lower_pos;
    case Snotra::ItemSizeRole:
        return itemdata->lower_len;
    case Snotra::ItemDataRole:
        return get_data(index);
    default:
        return QVariant();
    }
}

QByteArray PowerTalkerReader::get_data(const QModelIndex &index) const {
    Snotra::dict_data *item = static_cast<Snotra::dict_data*>(index.internalPointer());
    if (!item->leaf) return QByteArray();

    QFile f;
    QByteArray t, decompressed_t;
    QString filename_tag_lower, filename_tag_upper;
    quint16 len;
    quint32 offset = 0, len2 = 0;
    int pos2 = 0, pos = 0;
    f.setFileName(this->aux_filename_1);
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();
    f.seek(0x52);
    for (int i = 0; i < 3; i++) {
        t = f.read(12);
        filename_tag_upper = QString::fromLatin1(t.mid(0, 8));
        pos = *(quint16 *)(t.constData() + 8);
        if (filename_tag_lower < item->data_tag && item->data_tag <= filename_tag_upper) {
            pos2 = pos;
        }
        filename_tag_lower = filename_tag_upper;
    }
    pos2 += f.pos();
    f.seek(pos2);
    t = f.read(6);
    len = *(quint16 *)(t.constData());
    t = f.read(len);
    LZMADecoder::decodeLZMA(t, decompressed_t);
    f.close();
    pos = 0;
    while (pos < decompressed_t.length()) {
        t = decompressed_t.mid(pos, 20);
        if (QString::fromLatin1(t.mid(0, 8)) == item->data_tag) {
            offset = *(quint32 *)(t.constData() + 12);
            len2 = *(quint32 *)(t.constData() + 16);
            break;
        }
        pos += 20;
    }
    f.setFileName(this->aux_filename_2);
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();
    f.seek(0x3a + offset);
    t = f.read(len2);
    f.close();
    return t;
}

QVariant PowerTalkerReader::headerData(int , Qt::Orientation orientation, int role) const {
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Horizontal) {
            return "word";
        }
    default:
        return QVariant();
    }
}

Qt::ItemFlags PowerTalkerReader::flags(const QModelIndex &index) const {
    if (!index.isValid()) return 0;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool PowerTalkerReader::canFetchMore(const QModelIndex &) const {
    return false;
}

void PowerTalkerReader::fetchMore(const QModelIndex &) {
    return;
}
