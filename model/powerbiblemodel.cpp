#include "include/powerbiblemodel.h"
#include "include/defs.h"
#include "lzmadecoder.h"
#include <QFile>
#include <QByteArray>
#include <QString>
#include <QTextCodec>
#include <QDataStream>
#include <QDebug>
#include <QVector>

PowerBibleModel::PowerBibleModel(QObject *parent) : QAbstractItemModel(parent) {

}

PowerBibleModel::PowerBibleModel(const QString &filename, Snotra::database_args args, QObject *parent): QAbstractItemModel(parent) {
    this->is_loaded = false;
    this->load(filename, args);
}

PowerBibleModel::~PowerBibleModel() {
}


bool PowerBibleModel::load(const QString &filename, Snotra::database_args args) {
    if (this->is_loaded) this->book_data.clear();
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
    this->header_data.size = header_len;

    QDataStream t(header);
    t.setByteOrder(QDataStream::BigEndian);
    t >> this->header_data.dictionary_info_size;
    t.setByteOrder(QDataStream::LittleEndian);
    this->header_data.dictionary_info.resize(this->header_data.dictionary_info_size);
    t.readRawData(this->header_data.dictionary_info.data(), this->header_data.dictionary_info_size);
    t >> this->header_data.build_date_size;
    this->header_data.build_date.resize(this->header_data.build_date_size);
    t.readRawData(this->header_data.build_date.data(), this->header_data.build_date_size);

    t.skipRawData(0x24);
    t >> this->header_data.index_11kb_start >> this->header_data.index_11kb_len;
    t.skipRawData(0x06);
    t >> this->header_data.data_start >> this->header_data.data_len;
    t >> this->header_data.csum;

    if (this->args == Snotra::NewTestament) {
        this->book_data.resize(27);
    } else if (this->args == Snotra::OldTestament) {
        this->book_data.resize(39);
    }
    f.seek(header_len);
    QByteArray u;
    for (int i = 0; i < 66; i++) {
        u = f.read(0x99);
        QDataStream v(u);
        v.setByteOrder(QDataStream::LittleEndian);
        quint16 book_id;
        quint8 verse_count, sentence_count;
        v >> book_id;
        if (this->args == Snotra::OldTestament) {
            if (!(book_id & 0x0100)) continue;
        } else {
            if (!(book_id & 0x0200)) continue;
        }
        v >> verse_count;
        this->book_data[(book_id & 0xff) - 1].chapter_verse.resize(verse_count);
        for (int j = 0; j < verse_count; j++) {
            v >> sentence_count;
            this->book_data[(book_id & 0xff) - 1].chapter_verse[j] = sentence_count;
        }
    }
    u = f.read(0x100);
    QDataStream v(u);
    v.setByteOrder(QDataStream::LittleEndian);
    for (int i = 0; i < 32; i++) {
        quint32 book_pos, sentence_id;
        v >> book_pos >> sentence_id;
        this->index_pos.push_back(book_pos);
        this->index_id.push_back(sentence_id);
    }

    f.close();
    this->is_loaded = true;
    return true;
}

bool PowerBibleModel::get_load_status() {
    return this->is_loaded;
}


QModelIndex PowerBibleModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();
    quint64 parent_id;
    if (!parent.isValid()) parent_id = 0;
    else parent_id = parent.internalId();

    if ((parent_id & 0xff) == 0) {
        quint8 id = ((parent_id & 0xff00) >> 8);
        if ((id - 1) > this->book_data.count()) return QModelIndex();
        if (id == 0) return createIndex(row, column, (quint32)((row + 1) << 8));
        else return createIndex(row, column, (quint32)((id << 8) | (row + 1)));
    } else {
        return QModelIndex();
    }
}

QModelIndex PowerBibleModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) return QModelIndex();
    quint64 id = child.internalId();

    if ((id & 0xff) != 0) {
        quint8 parent_id = ((id & 0xff00) >> 8);
        return createIndex(parent_id - 1, 0, (quint32)(parent_id << 8));
    } else {
        return QModelIndex();
    }
}

int PowerBibleModel::rowCount(const QModelIndex &parent) const {
    if (parent.column() > 0) return 0;
    if (!parent.isValid()) return this->book_data.count();
    quint64 id = parent.internalId();
    if (id & 0xff) return 0;
    else {
        quint8 upper_id = (id & 0xff00) >> 8;
        upper_id--;
        return this->book_data[upper_id].chapter_verse.count();
    }
}

int PowerBibleModel::columnCount(const QModelIndex &) const {
    return 1;
}

bool PowerBibleModel::hasChildren(const QModelIndex &parent) const {
    if (!parent.isValid()) return true;
    quint64 id = parent.internalId();
    return (!(id & 0xff));
}

QVariant PowerBibleModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    quint64 id = index.internalId();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if ((id & 0xff) == 0) {
            return tr("Book %1").arg((id & 0xff00) >> 8);
        } else {
            return tr("Chapter %1").arg(id & 0xff);
        }
    case Snotra::ItemPositionRole:
        return 0;
    case Snotra::ItemSizeRole:
        return 0;
    case Snotra::ItemDataRole:
        return get_chapter((id & 0xff00) >> 8, id & 0xff);
    default:
        return QVariant();
    }
}

QString PowerBibleModel::get_chapter(const int book, const int chapter) const {
    if (book <= 0 || chapter <= 0) return QString();
    quint32 chapter_id = (book << 8) | chapter;
    if (this->args == Snotra::OldTestament) chapter_id |= (1 << 16);
    else chapter_id |= (2 << 16);
    chapter_id <<= 8;

    QVector<quint32>::const_iterator lbi = qLowerBound(this->index_id, chapter_id);
    QVector<quint32>::const_iterator ubi = qUpperBound(this->index_id, chapter_id + 0x0100);
    int lb = std::distance(this->index_id.begin(), lbi);
    int ub = std::distance(this->index_id.begin(), ubi);

    quint32 pos;
    QByteArray u, u2, body_compressed, body_uncompressed;
    quint32 file_pos, sentence_id, file_pos_prev = 0xcdcdcdcd;
    quint16 file_internal_pos;
    int sentence_end;
    QTextCodec *codec = QTextCodec::codecForName("UTF-16LE");
    QString ret, t, t2;
    QChar zero = '\0';

    QFile f(this->filename);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    for (int i = lb; i <= ub; i++) {
        pos = this->header_data.index_11kb_start + this->index_pos[i];
        f.seek(pos);
        u = f.read(6);
        u = f.read(*(quint16 *)(u.constData()));
        LZMADecoder::decode_lzma_2(u, u2);
        QDataStream u3(u2);
        u3.setByteOrder(QDataStream::LittleEndian);

        while (!u3.atEnd()) {
            u3 >> file_pos >> file_internal_pos >> sentence_id;
            if (file_pos == 0xcdcdcdcd) break;
            if ((sentence_id & 0xffffff00) == chapter_id) {
                if (file_pos_prev != file_pos) {
                    f.seek(this->header_data.data_start + file_pos);
                    body_compressed = f.read(6);
                    body_compressed = f.read(*(quint16 *)(body_compressed.constData()));
                    LZMADecoder::decode_lzma_2(body_compressed, body_uncompressed);
                    t2 = codec->toUnicode(body_uncompressed);
                }
                sentence_end = t2.indexOf(zero, file_internal_pos / 2);
                if (sentence_end < 0) sentence_end = t2.length();
                t = t2.mid(file_internal_pos / 2, (sentence_end - file_internal_pos / 2));
                ret.append(t);
                ret.append("\n");
                file_pos_prev = file_pos;
            }
        }
    }
    f.close();
    return ret;
}

QVariant PowerBibleModel::headerData(int , Qt::Orientation orientation, int role) const {
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Horizontal) {
            return "word";
        }
    default:
        return QVariant();
    }
}

Qt::ItemFlags PowerBibleModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) return 0;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
