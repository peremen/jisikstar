#include "mathdictreader.h"
#include "defs.h"
#include "lzmadecoder.h"
#include <QFile>
#include <QByteArray>
#include <QString>
#include <QTextCodec>
#include <QDebug>
#include <QPixmap>
#include <QDataStream>

MathDictReader::MathDictReader(QObject *parent) : QObject(parent) {
    root_data = new Snotra::dict_data;
    this->is_loaded = false;
}


MathDictReader::MathDictReader(const QString &filename, QObject *parent): QObject(parent) {
    root_data = new Snotra::dict_data;
    this->is_loaded = false;
    this->load(filename);
}

MathDictReader::~MathDictReader() {
    delete root_data;
}

bool MathDictReader::load(const QString &filename) {
    if (this->is_loaded) delete root_data;
    QFile f(filename);
    if (!f.exists()) return false;
    this->filename = filename;

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
    t >> this->header_data.build_date_size;
    this->header_data.build_date.resize(this->header_data.build_date_size);
    t.readRawData(this->header_data.build_date.data(), this->header_data.build_date_size);
    t >> this->header_data.build_time_size;
    this->header_data.build_time.resize(this->header_data.build_time_size);
    t.readRawData(this->header_data.build_time.data(), this->header_data.build_time_size);
    t >> this->header_data.unk >> this->header_data.header_pos >> this->header_data.header_len;
    t >> this->header_data.data_pos >> this->header_data.csum;

    f.seek(this->header_data.size);

    quint16 compressed_size, decompressed_size;
    f.read((char *)&compressed_size, sizeof(compressed_size));
    f.read((char *)&decompressed_size, sizeof(decompressed_size));
    QByteArray compressed_index, decompressed_index;
    this->is_loaded = true;
    f.close();

    f.read(compressed_index.data(), compressed_size);
    decompressed_index.resize(decompressed_size);
    decompressed_index.clear();
    LZMADecoder::decodeLZMA(compressed_index.data(), compressed_size,
                            decompressed_index.data(), decompressed_size);
    this->clear_root_data();

    quint16 pre_index_size = *(quint16 *)decompressed_index.constData();
    int pos = sizeof(quint16);
    this->root_data->lower_len = pre_index_size;
#if QT_VERSION >= 0x040700
    this->root_data->lower_data.reserve(pre_index_size);
#endif
    QTextCodec *codec = QTextCodec::codecForName("UTF-16LE");

    Snotra::dict_data *newlevel, *newlevel2;
    for (quint16 i = 0; i < pre_index_size; i++) {
        quint16 index_len = *(quint16 *)(decompressed_index.constData() + pos);
        quint16 index_pos = *(quint16 *)(decompressed_index.constData() + pos + 2);
        QString index_desc = codec->toUnicode(decompressed_index.mid(pos + 4, Snotra::PREINDEX_SIZE_FORMULA - 4));
        index_desc.remove(QChar('\0'));

        newlevel = new Snotra::dict_data;
        newlevel->row = i;
        newlevel->desc = index_desc;
        newlevel->lower_pos = index_pos;
        newlevel->lower_len = index_len;
#if QT_VERSION >= 0x040700
        newlevel->lower_data.reserve(index_len);
#endif
        newlevel->leaf = false;
        newlevel->parent = this->root_data;
        this->root_data->lower_data.append(newlevel);

        pos = 2 + Snotra::PREINDEX_SIZE_FORMULA * pre_index_size + Snotra::INDEX_SIZE_FORMULA * newlevel->lower_pos;
        for (quint16 j = 0; j < newlevel->lower_len; ++j) {
            quint32 item_pos = *(quint32 *)(decompressed_index.constData() + pos);
            quint32 item_len = *(quint32 *)(decompressed_index.constData() + pos + 4);
            QString item_desc = codec->toUnicode(decompressed_index.mid(pos + 8, Snotra::PREINDEX_SIZE_FORMULA - 8));
            item_desc.remove(QChar('\0'));

            newlevel2 = new Snotra::dict_data;
            newlevel2->row = j;
            newlevel2->desc = item_desc;
            newlevel2->lower_len = item_len;
            newlevel2->lower_pos = item_pos;
            newlevel2->parent = newlevel;
            newlevel2->leaf = true;
            newlevel->lower_data.append(newlevel2);
            pos += Snotra::INDEX_SIZE_FORMULA;
        }
        pos = 2 + (i + 1) * Snotra::PREINDEX_SIZE_FORMULA;
        newlevel->lower_loaded = true;
    }
    this->root_data->lower_loaded = true;
    return true;
}

bool MathDictReader::isLoaded() {
    return this->is_loaded;
}


void MathDictReader::clear_root_data() {
    this->root_data->row = 0;
    this->root_data->lower_data.clear();
    this->root_data->desc.clear();
    this->root_data->lower_len = 0;
    this->root_data->lower_loaded = false;
    this->root_data->lower_pos = 0;
    this->root_data->parent = this->root_data;
    this->root_data->leaf = false;
}

bool MathDictReader::exportToFile(const QString &destination, Snotra::FileType type) {
    QFile f(this->filename);
    Snotra::dict_data *level1, *level2;
    QString itemName;
    QByteArray pngData;

    foreach (level1, this->root_data->lower_data) {
        foreach (level2, level1->lower_data) {
            itemName = QString("%1/%2").arg(level1->desc, level2->desc);
            qDebug() << itemName;
            f.seek(this->header_data.data_pos + level2->lower_pos);
            pngData = f.read(level2->lower_len);
        }
    }

    if (!f.open(QIODevice::ReadOnly)) return false;

    switch (type) {
    case Snotra::Tab:
        break;
    case Snotra::Babylon:
        break;
    case Snotra::Stardict:
        break;
    }

    f.close();
    return true;
}
