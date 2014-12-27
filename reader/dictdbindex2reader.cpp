#include "dictdbindex2reader.h"
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

DictDBIndex2Reader::DictDBIndex2Reader(QObject *parent) : QObject(parent) {
}

DictDBIndex2Reader::DictDBIndex2Reader(const QString &filename, QObject *parent): QObject(parent) {
    this->load(filename);
}

bool DictDBIndex2Reader::load(const QString &filename) {
    QFile f;
    f.setFileName(filename);
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
    t >> this->header_data.compindex_1_start >> this->header_data.compindex_1_size;
    t.skipRawData(0x0b);
    t >> this->header_data.preindex_2_count >> this->header_data.preindex_2_size;
    t >> this->header_data.compindex_2_start >> this->header_data.compindex_2_size;
    t >> this->header_data.index_8b_start >> this->header_data.compindex_3_size;
    t >> this->header_data.index_8b_count >> this->header_data.index_8b_size;
    t >> this->header_data.compindex_3_start >> this->header_data.compindex_3_len;
    t >> this->header_data.unk;
    t >> this->header_data.data_size >> this->header_data.data_start >> this->header_data.data_len;
    t >> this->header_data.csum;
    f.seek(this->header_data.size);

    this->preindex[0].clear();
    this->preindex[1].clear();
#if QT_VERSION >= 0x040700
    this->preindex[0].reserve(this->header_data.preindex_1_count);
    this->preindex[1].reserve(this->header_data.preindex_1_count);
#endif
    QByteArray decompressed_index;
    QString index_desc;

    for (unsigned int i = 0; i < this->header_data.preindex_1_count; ++i) {
        decompressed_index = f.read(this->header_data.preindex_1_size);
        if (decompressed_index.length() != this->header_data.preindex_1_size) return false;
        Snotra::preindex_data d;
        index_desc = this->codec->toUnicode(decompressed_index.mid(4));
        d.key = index_desc.remove(QChar('\0'));
        d.index_loaded = false;
        d.index_pos =  *(quint32 *)(decompressed_index.constData());
        d.index_len = 0;
        this->preindex[0].append(d);
    }

    for (unsigned int i = 0; i < this->header_data.preindex_2_count; ++i) {
        decompressed_index = f.read(this->header_data.preindex_2_size);
        if (decompressed_index.length() != this->header_data.preindex_2_size) return false;
        Snotra::preindex_data d;
        index_desc = this->codec->toUnicode(decompressed_index.mid(4));
        d.key = index_desc.remove(QChar('\0'));
        d.index_loaded = false;
        d.index_pos = *(quint32 *)(decompressed_index.constData());
        d.index_len = 0;
        this->preindex[1].append(d);
    }
    f.close();
    this->populatePreindex(0);
    this->populatePreindex(1);
    this->populate10KIndex();
    this->populateDictionaryData();
    return true;
}

void DictDBIndex2Reader::populatePreindex(int preindexId) {
    QByteArray compressed_index, decompressed_index;
    quint16 decompressed_size_16, compressed_size_16, dummy_16;
    quint32 decompressed_size, compressed_size, dummy;
    quint64 seek_pos, seekPosBase;
    int preindexCount;
    QFile f;

    f.setFileName(this->filename);
    if (!f.open(QIODevice::ReadOnly)) return;
    if (preindexId == 0) {
        seekPosBase = this->header_data.compindex_1_start;
        preindexCount = this->header_data.preindex_1_count;
    } else {
        seekPosBase = this->header_data.compindex_2_start;
        preindexCount = this->header_data.preindex_2_count;
    }

    for (int i = 0; i < preindexCount; ++i) {
        seek_pos = seekPosBase + this->preindex[preindexId].at(i).index_pos;
        f.seek(seek_pos);
        f.read((char *)&compressed_size_16, sizeof(compressed_size_16));
        f.read((char *)&decompressed_size_16, sizeof(decompressed_size_16));
        f.read((char *)&dummy_16, sizeof(dummy_16));
        if (decompressed_size_16 <= compressed_size_16) {
            // Those values are UInt32, retry it
            f.seek(seek_pos);
            f.read((char *)&compressed_size, sizeof(compressed_size));
            f.read((char *)&decompressed_size, sizeof(decompressed_size));
            f.read((char *)&dummy, sizeof(dummy));
        } else {
            // Those values are 16-bit, go ahead
            decompressed_size = decompressed_size_16;
            compressed_size = compressed_size_16;
            dummy = dummy_16;
        }

        compressed_index.clear();
        compressed_index.resize(compressed_size);
        decompressed_index.clear();
        decompressed_index.resize(decompressed_size);

        f.read(compressed_index.data(), compressed_size);
        if (!LZMADecoder::decodeLZMA(compressed_index.data(),
                                     compressed_size,
                                     decompressed_index.data(),
                                     decompressed_size)) return;

        int pos = 0;
        quint8 len_str; QString index_desc;
        while (1) {
            Snotra::dict_data_2 d;
            len_str = *(quint8 *)(decompressed_index.constData() + pos + 4);
            if (this->utf16) index_desc = this->codec->toUnicode(decompressed_index.mid(pos + 5, len_str + 2));
            else index_desc = this->codec->toUnicode(decompressed_index.mid(pos + 5, len_str + 1));
            d.data_file_pos = *(quint32 *)(decompressed_index.constData() + pos);
            d.data_file_pos &= msb_mask;
            d.desc = index_desc.remove(QChar('\0'));
            if (this->utf16) pos += (5 + len_str + 2);
            else pos += (5 + len_str + 1);
            this->wordTo10KIndex[preindexId].append(qMakePair(d.desc, d.data_file_pos));
            if (pos >= decompressed_index.size()) break;
        }
        this->preindex[preindexId][i].index_loaded = true;
    }

    f.close();
}

void DictDBIndex2Reader::populate10KIndex() {
    //int r = index.row();
    QByteArray t, decompressed_t;
    quint16 decompressed_size_16, compressed_size_16, dummy_16;
    quint32 decompressed_size, compressed_size, dummy;
    quint32 pos, pos2;
    bool is_len_uint32 = false;
    quint16 len;
    quint32 len_32;
    int tenk_idx_count = 0;
    QFile f;

    f.setFileName(this->filename);
    if (!f.open(QIODevice::ReadOnly)) return;

    this->tenKHashMap.reserve(this->header_data.index_8b_count * 1000);
    for (int i = 0; i < this->header_data.index_8b_count; ++i) {
        f.seek(this->header_data.index_8b_start + i * 2 * sizeof(quint32));
        f.read((char *)&pos, sizeof(pos));
        f.read((char *)&pos2, sizeof(pos2));
        pos2 &= msb_mask;
        //if (pos2 > (this->loaded_data.at(r).data_file_pos)) break;

        if (i == 0) {
            f.seek(this->header_data.compindex_3_start + pos);
            f.read((char *)&compressed_size_16, sizeof(compressed_size_16));
            f.read((char *)&decompressed_size_16, sizeof(decompressed_size_16));
            f.read((char *)&dummy_16, sizeof(dummy_16));
            if (decompressed_size_16 <= compressed_size_16) is_len_uint32 = true;
        }

        f.seek(this->header_data.compindex_3_start + pos);
        if (is_len_uint32) {
            // Those values are UInt32, retry it
            f.read((char *)&compressed_size, sizeof(compressed_size));
            f.read((char *)&decompressed_size, sizeof(decompressed_size));
            f.read((char *)&dummy, sizeof(dummy));
        } else {
            // Those values are 16-bit, go ahead
            f.read((char *)&compressed_size_16, sizeof(compressed_size_16));
            f.read((char *)&decompressed_size_16, sizeof(decompressed_size_16));
            f.read((char *)&dummy_16, sizeof(dummy_16));
            decompressed_size = decompressed_size_16;
            compressed_size = compressed_size_16;
            dummy = dummy_16;
        }

        t.clear();
        t.resize(compressed_size);
        decompressed_t.clear();
        decompressed_t.resize(decompressed_size);

        f.read(t.data(), compressed_size);
        if (!LZMADecoder::decodeLZMA(t.data(), compressed_size,
                                     decompressed_t.data(),
                                     decompressed_size)) return;

        if (i == 0) {
            if (decompressed_t.size() == 12000) this->is_len_10k_uint32 = true;
            else this->is_len_10k_uint32 = false;
        }

        tenk_idx_count = decompressed_t.size() / (this->is_len_10k_uint32 ? 12 : 10);
        for (int j = 0; j < tenk_idx_count; ++j) {
            if (this->is_len_10k_uint32) {
                pos = *(quint32 *)(decompressed_t.constData() + 12 * j);
                len_32 = *(quint16 *)(decompressed_t.constData() + 12 * j + 4);
                pos2 = *(quint32 *)(decompressed_t.constData() + 12 * j + 8);
            } else {
                pos = *(quint32 *)(decompressed_t.constData() + 10 * j);
                len = *(quint16 *)(decompressed_t.constData() + 10 * j + 4);
                len_32 = len;
                pos2 = *(quint32 *)(decompressed_t.constData() + 10 * j + 6);
            }
            pos2 &= msb_mask;
            this->tenKHashMap.insert(pos2, qMakePair(pos, len_32));
        }
    }
    f.close();
}

void DictDBIndex2Reader::populateDictionaryData() {
    quint16 decompressed_size_16, compressed_size_16, dummy_16;
    quint32 decompressed_size, compressed_size, dummy, start_pos;
    QByteArray t, *decompressed_t;
    QFile f;

    f.setFileName(this->filename);
    if (!f.open(QIODevice::ReadOnly)) return;

    f.seek(this->header_data.data_start);
    while (!f.atEnd()) {
        start_pos = f.pos() - this->header_data.data_start;
        if (this->is_len_10k_uint32) {
            // Those values are UInt32
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

        decompressed_t = new QByteArray();
        t.clear();
        t.resize(compressed_size);
        decompressed_t->clear();
        decompressed_t->resize(decompressed_size);

        f.read(t.data(), compressed_size);
        if (!LZMADecoder::decodeLZMA(t.data(), compressed_size,
                                     decompressed_t->data(),
                                     decompressed_size)) return;
        this->compDataIndex.insert(start_pos, *decompressed_t);
    }
    f.close();
}

bool DictDBIndex2Reader::exportToFile(const QString &destination,
                                      Snotra::FileType type,
                                      int preindexId) {
    QPair<QString, quint32> i;
    QPair<quint32, quint32> compPosLen;
    QString fileLine, defn;
    QFile f;
    int idx;

    f.setFileName(destination);
    if (!f.open(QIODevice::WriteOnly)) return false;

    foreach (i, this->wordTo10KIndex[preindexId]) {
        compPosLen = this->tenKHashMap.value(i.second);
        defn = this->codec->toUnicode(this->compDataIndex.value(compPosLen.first).mid(compPosLen.second));
        idx = defn.indexOf(QChar());
        if (idx > 0) defn.chop(defn.length() - idx);
        switch (type) {
        case Snotra::Tab:
            defn.replace('\n', "<br>");
            defn = Snotra::formatDioDictString(defn);
            fileLine = QString("%1\t%2\n").arg(i.first).arg(defn);
            f.write(fileLine.toUtf8());
            break;
        case Snotra::Babylon:
            defn.replace('\n', "<br>");
            defn = Snotra::formatDioDictString(defn);
            fileLine = QString("%1\n%2\n\n").arg(i.first).arg(defn);
            f.write(fileLine.toUtf8());
            break;
        case Snotra::Stardict:
            break;
        }
    }
    f.close();
    return true;

}
