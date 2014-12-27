#include "dictdbindex1model.h"
#include "defs.h"
#include "lzmadecoder.h"
#include <QFile>
#include <QByteArray>
#include <QString>
#include <QTextCodec>
#include <QDebug>
#include <QDataStream>
#include <QtAlgorithms>
#include <QtGlobal>

DictDBIndex1Model::DictDBIndex1Model(QObject *parent) : QAbstractListModel(parent) {
    this->keyword_pos_loaded = -1; this->keyword_pos_lower = -1; this->keyword_pos_upper = -1;
    this->is_loaded = false;

    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "title";
    roles[Snotra::ItemDataRole] = "itemData";
#if QT_VERSION < 0x050000
    setRoleNames(roles);
#endif
}

DictDBIndex1Model::DictDBIndex1Model(const QString &filename, QObject *parent): QAbstractListModel(parent) {
    this->keyword_pos_loaded = -1; this->keyword_pos_lower = -1; this->keyword_pos_upper = -1;
    this->is_loaded = false;
    this->load(filename);

    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "title";
    roles[Snotra::ItemDataRole] = "itemData";
#if QT_VERSION < 0x050000
    setRoleNames(roles);
#endif
}

DictDBIndex1Model::~DictDBIndex1Model() {

}

bool DictDBIndex1Model::load(const QString &filename) {
    QFile f(filename);
    if (!f.exists()) return false;
    this->filename = filename;
    if (this->filename.indexOf("UCS2LE") > 0) {
        this->codec = QTextCodec::codecForName("UTF-16LE");
        this->utf16 = true;
    } else if (this->filename.indexOf("KSCS") > 0) {
        this->codec = QTextCodec::codecForName("CP949");
        this->utf16 = false;
    } else { // fallback, should be UCS2?
        this->codec = QTextCodec::codecForName("UTF-16LE");
        this->utf16 = true;
    }

    if (!f.open(QIODevice::ReadOnly)) return false;
    QDataStream s(&f);
    s.setByteOrder(QDataStream::LittleEndian);
    quint8 header_len;
    s >> header_len;
    QByteArray header;
    header.resize(header_len);
    s.readRawData(header.data(), header_len);
    this->header_data.size = header_len;

    QDataStream t(header);
    t.setByteOrder(QDataStream::LittleEndian);
    t >> this->header_data.dictionary_info_size;
    this->header_data.dictionary_info.resize(this->header_data.dictionary_info_size);
    t.readRawData(this->header_data.dictionary_info.data(), this->header_data.dictionary_info_size);
    t >> this->header_data.version_info_size;
    this->header_data.version_info.resize(this->header_data.version_info_size);
    t.readRawData(this->header_data.version_info.data(), this->header_data.version_info_size);
    t.skipRawData(0x03);
    t >> this->header_data.build_info_size;
    this->header_data.build_info.resize(this->header_data.build_info_size);
    t.readRawData(this->header_data.build_info.data(), this->header_data.build_info_size);
    t.skipRawData(0x24);
    t >> this->header_data.preindex_1_count >> this->header_data.preindex_1_size;
    t >> this->header_data.compindex_1_start >> this->header_data.compindex_1_size >> this->header_data.compindex_1_end;
    t >> this->header_data.index_10kb_len >> this->header_data.index_10kb_count >> this->header_data.extra3;
    t >> this->header_data.index_10kb_start >> this->header_data.index_10kb_size;
    t >> this->header_data.unk_2 >> this->header_data.unk_3;
    t >> this->header_data.data_start >> this->header_data.data_end;
    t >> this->header_data.csum;

    f.seek(this->header_data.size);
    this->loaded_data.clear();
    this->preindex.clear();
#if QT_VERSION >= 0x040700
    this->preindex.reserve(this->header_data.preindex_1_count);
#endif
    QByteArray decompressed_index;
    for (unsigned int i = 0; i < this->header_data.preindex_1_count; ++i) {
        decompressed_index = f.read(this->header_data.preindex_1_size);
        if (decompressed_index.length() != this->header_data.preindex_1_size) break;
        Snotra::preindex_data d;
        QString index_desc = this->codec->toUnicode(decompressed_index.mid(4));
        d.key = index_desc.remove(QChar('\0'));
        d.index_loaded = false;
        d.index_pos = *(quint32 *)(decompressed_index.constData());
        d.index_len = 0;
        this->preindex.append(d);
    }
    this->keyword_pos_lower = 0;
    this->keyword_pos_upper = this->header_data.preindex_1_count;
    this->keyword_pos_loaded = this->keyword_pos_lower;
    f.close();
    return true;
}

bool DictDBIndex1Model::get_load_status() {
    return this->is_loaded;
}

int DictDBIndex1Model::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    } else {
        return loaded_data.size();
    }
}

QVariant DictDBIndex1Model::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return this->loaded_data.at(index.row()).desc;
    case Snotra::ItemPositionRole:
        return this->loaded_data.at(index.row()).data_file_pos;
    case Snotra::ItemSizeRole:
        return this->loaded_data.at(index.row()).data_file_len;
    case Snotra::ItemDataRole:
        return get_data(index);
    default:
        return QVariant();
    }
}

QString DictDBIndex1Model::get_data(const QModelIndex &index) const {
    int r = index.row();
    QFile f(this->filename);
    if (!f.open(QIODevice::ReadOnly)) return QString();

    f.seek(this->header_data.compindex_1_end);
    quint32 pos, pos2;
    for (unsigned int i = 0; i < this->header_data.index_10kb_count; ++i) {
        f.read((char *)&pos, sizeof(pos));
        f.read((char *)&pos2, sizeof(pos2));
        pos2 &= msb_mask;
        if (pos2 > (this->loaded_data.at(r).data_file_pos)) break;
    }

    f.seek(this->header_data.index_10kb_start + pos);
    QByteArray t, decompressed_t;
    quint16 decompressed_size_16, compressed_size_16, dummy_16;
    quint32 decompressed_size, compressed_size, dummy;
    f.read((char *)&compressed_size_16, sizeof(compressed_size_16));
    f.read((char *)&decompressed_size_16, sizeof(decompressed_size_16));
    f.read((char *)&dummy_16, sizeof(dummy_16));
    if (decompressed_size_16 <= compressed_size_16) {
        // Those values are UInt32, retry it
        f.seek(this->header_data.index_10kb_start + pos);
        f.read((char *)&compressed_size, sizeof(compressed_size));
        f.read((char *)&decompressed_size, sizeof(decompressed_size));
        f.read((char *)&dummy, sizeof(dummy));
    } else {
        // Those values are 16-bit, go ahead
        decompressed_size = decompressed_size_16;
        compressed_size = compressed_size_16;
        dummy = dummy_16;
    }

    t = f.read(compressed_size);
    if (!LZMADecoder::decode_lzma_2(t, decompressed_t)) return QString();

    quint16 len;
    quint32 len_32;
    bool is_len_32bit = (decompressed_t.size() == 12000);
    for (int i = 0; i < 1000; ++i) {
        if (is_len_32bit) {
            pos = *(quint32 *)(decompressed_t.constData() + 12 * i);
            len_32 = *(quint32 *)(decompressed_t.constData() + 12 * i + 4);
            pos2 = *(quint32 *)(decompressed_t.constData() + 12 * i + 8);
        } else {
            pos = *(quint32 *)(decompressed_t.constData() + 10 * i);
            len = *(quint16 *)(decompressed_t.constData() + 10 * i + 4);
            len_32 = len;
            pos2 = *(quint32 *)(decompressed_t.constData() + 10 * i + 6);
        }
        pos2 &= msb_mask;
        if (pos2 == (this->loaded_data.at(r).data_file_pos)) break;
    }

    f.seek(this->header_data.data_start + pos);
    if (is_len_32bit) {
        f.seek(this->header_data.data_start + pos);
        f.read((char *)&compressed_size, sizeof(compressed_size));
        f.read((char *)&decompressed_size, sizeof(decompressed_size));
        f.read((char *)&dummy, sizeof(dummy));
    } else {
        // Those values are 16-bit
        f.read((char *)&compressed_size_16, sizeof(compressed_size_16));
        f.read((char *)&decompressed_size_16, sizeof(decompressed_size_16));
        f.read((char *)&dummy_16, sizeof(dummy_16));

        decompressed_size = decompressed_size_16;
        compressed_size = compressed_size_16;
        dummy = dummy_16;
    }
    t.clear();
    t = f.read(compressed_size);
    if (!LZMADecoder::decode_lzma_2(t, decompressed_t)) return QString();
    f.close();

    QString result_str = this->codec->toUnicode(decompressed_t.mid(len_32));
    int idx = result_str.indexOf(QChar());
    if (idx > 0) result_str.chop(result_str.length() - idx);
    return result_str;
}

QVariant DictDBIndex1Model::headerData(int , Qt::Orientation orientation, int role) const {
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Horizontal) {
            return "word";
        }
    default:
        return QVariant();
    }
}

Qt::ItemFlags DictDBIndex1Model::flags(const QModelIndex &index) const {
    if (!index.isValid()) return 0;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool DictDBIndex1Model::canFetchMore(const QModelIndex &) const {
    bool ret = true;
    for (int i = this->keyword_pos_lower; i < this->keyword_pos_upper; i++) {
        ret &= this->preindex.at(i).index_loaded;
    }
    return !ret;
}

void DictDBIndex1Model::fetchMore(const QModelIndex &) {
    if (this->keyword_pos_loaded >= this->keyword_pos_upper) return;

    QByteArray compressed_index, decompressed_index;
    quint16 decompressed_size_16, compressed_size_16, dummy_16;
    quint32 decompressed_size, compressed_size, dummy;

    QFile f(this->filename);
    if (!f.open(QIODevice::ReadOnly)) return;
    f.seek(this->header_data.compindex_1_start + this->preindex.at(this->keyword_pos_loaded).index_pos);
    f.read((char *)&compressed_size_16, sizeof(compressed_size_16));
    f.read((char *)&decompressed_size_16, sizeof(decompressed_size_16));
    f.read((char *)&dummy_16, sizeof(dummy_16));
    if (decompressed_size_16 <= compressed_size_16) {
        // Those values are UInt32, retry it
        f.seek(this->header_data.compindex_1_start + this->preindex.at(this->keyword_pos_loaded).index_pos);
        f.read((char *)&compressed_size, sizeof(compressed_size));
        f.read((char *)&decompressed_size, sizeof(decompressed_size));
        f.read((char *)&dummy, sizeof(dummy));
    } else {
        // Those values are 16-bit, go ahead
        decompressed_size = decompressed_size_16;
        compressed_size = compressed_size_16;
        dummy = dummy_16;
    }
    compressed_index = f.read(compressed_size);
    f.close();

    Q_ASSERT(compressed_index.size() == compressed_size);
    if (!LZMADecoder::decode_lzma_2(compressed_index, decompressed_index)) return;

    QList<Snotra::dict_data_2> new_data;
    int pos = 0;
    int original_size = this->loaded_data.size();
    quint8 len_str; QString index_desc;
    while (1) {
        Snotra::dict_data_2 d;
        len_str = *(quint8 *)(decompressed_index.constData() + pos + 4);
        if (this->utf16) index_desc = this->codec->toUnicode(decompressed_index.mid(pos + 5, len_str + 2));
        else index_desc = this->codec->toUnicode(decompressed_index.mid(pos + 5, len_str + 1));
        d.data_file_pos = *(quint32 *)(decompressed_index.constData() + pos);
        d.data_file_pos &= msb_mask;
        d.desc =  index_desc.remove(QChar('\0'));
        if (this->utf16) pos += (5 + len_str + 2);
        else pos += (5 + len_str + 1);
        if (this->keyword.isEmpty()) {
            new_data.append(d);
            /*
            this->loaded_data.append(d);
            after_size++;
            */
        } else {
            int c = QString::compare(this->keyword, Snotra::sort_key(d.desc).left(this->keyword.length()), Qt::CaseInsensitive);
            if (c == 0) {
                new_data.append(d);
                /*
                this->loaded_data.append(d);
                after_size++;
                */
            }
        }
        if (pos >= decompressed_index.size()) break;
    }
    this->beginInsertRows(QModelIndex(), original_size, original_size + new_data.size() - 1);
    this->loaded_data.append(new_data);
    this->endInsertRows();
    this->preindex[this->keyword_pos_loaded].index_loaded = true;
    this->keyword_pos_loaded++;
}

void DictDBIndex1Model::init_keyword() {
    this->keyword = "";
    this->keyword_pos_loaded = 0;
    this->keyword_pos_lower = 0;
    this->keyword_pos_upper = this->preindex.size();
    this->clear_all();
    //reset();
    emit keywordChanged("");
}

void DictDBIndex1Model::set_keyword(const QString &keyword) {
    if (keyword.isEmpty()) return this->init_keyword();
    QString old_keyword = this->keyword;
    this->keyword = keyword;
    QList<Snotra::preindex_data>::iterator lower, upper;
    int new_lower, new_upper = 0;

    Snotra::preindex_data ref;
    ref.key = keyword;
    lower = qLowerBound(this->preindex.begin(), this->preindex.end(), ref, Snotra::compare_index());
    new_lower = std::distance(this->preindex.begin(), lower);
    if (this->keyword_pos_lower >= this->preindex.size()) {
        this->keyword_pos_lower = -1;
        this->keyword_pos_upper = -1;
        this->keyword_pos_loaded = 1;
    } else if (this->keyword_pos_lower == this->preindex.size() - 1) {
        this->keyword_pos_upper = this->preindex.size();
        this->keyword_pos_loaded = this->keyword_pos_lower;
    } else {
        upper = qUpperBound(lower + 1, this->preindex.end(), ref, Snotra::compare_index());
        int diff = std::distance(this->preindex.begin(), upper);
        new_upper = qMin(diff, this->preindex.size());

    }
    if (this->keyword_pos_lower == new_lower) {
        bool recycle = old_keyword.length() < this->keyword.length() && old_keyword.startsWith(this->keyword);
        if (this->keyword_pos_upper <= new_upper && recycle) {
            this->keyword_pos_upper = new_upper;
            if (this->keyword_pos_loaded > this->keyword_pos_upper) this->keyword_pos_loaded = this->keyword_pos_upper;
            this->refresh_index();
        } else {
            this->keyword_pos_upper = new_upper;
            this->keyword_pos_loaded = this->keyword_pos_lower;
            this->clear_all();
        }
    } else {
        this->keyword_pos_lower = new_lower;
        this->keyword_pos_upper = new_upper;
        this->keyword_pos_loaded = this->keyword_pos_lower;
        this->clear_all();
    }

    //reset();
    emit keywordChanged(this->keyword);
}

QString DictDBIndex1Model::get_keyword() {
    return this->keyword;
}

void DictDBIndex1Model::clear_all() {
    this->beginResetModel();
    //this->beginRemoveRows(QModelIndex(), 0, this->loaded_data.size()-1);
    this->loaded_data.clear();
    for (int i = 0; i < this->preindex.size(); ++i) this->preindex[i].index_loaded = false;
    this->endResetModel();
    //this->endRemoveRows();
}

void DictDBIndex1Model::refresh_index() {
    Snotra::dict_data_2 d;
    d.desc = this->keyword;
    QList<Snotra::dict_data_2>::iterator lower, upper;
    lower = qLowerBound(this->loaded_data.begin(), this->loaded_data.end(), d, Snotra::compare_index());
    int dist = std::distance(this->loaded_data.begin(), lower);
    if (dist > 0) {
        this->beginRemoveRows(QModelIndex(), 0, dist - 1);
        this->loaded_data.erase(this->loaded_data.begin(), lower);
        this->endRemoveRows();
    }
    upper = qUpperBound(this->loaded_data.begin(), this->loaded_data.end(), d, Snotra::compare_index());
    dist = std::distance(upper, this->loaded_data.end());
    if (dist > 0) {
        this->beginRemoveRows(QModelIndex(), std::distance(this->loaded_data.begin(), upper), this->loaded_data.size() - 1);
        this->loaded_data.erase(upper, this->loaded_data.end());
        this->endRemoveRows();
    }

}

int DictDBIndex1Model::get_preindex_count() {
    return 1;
}
