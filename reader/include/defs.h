#ifndef DEFS_H
#define DEFS_H

#include <Qt>
#include <QString>
#include <QList>
#include <QByteArray>

const quint32 msb_mask = 0xfffffff;

namespace Snotra {
enum database_type {
    Unknown = 0,
    DicDB,
    MathDB,
    PowerBible,
    PowerTalker,
    Other
};

enum database_args {
    none = 0,
    english,
    japanese,
    chinese,
    OldTestament,
    NewTestament
};

enum viewer_mode {
    Text = 0,
    Image,
    Audio
};

enum FileType {
    Tab = 0,
    Babylon,
    Stardict,
};

const int PREINDEX_SIZE_FORMULA = 0x44;
const int INDEX_SIZE_FORMULA = 0x48;
enum model_roles {
    ItemPositionRole = Qt::UserRole,
    ItemSizeRole,
    ItemDataRole
};

class dict_data {
public:
    int row, lower, upper;
    QString desc;
    QString data_tag;
    quint64 lower_pos;
    quint64 lower_len;
    bool leaf;
    bool lower_loaded;
    QList<dict_data*> lower_data;
    dict_data *parent;
    ~dict_data() {
        qDeleteAll(this->lower_data);
    }
};

struct preindex_data {
    QString key;
    quint32 index_pos, index_len;
    bool index_loaded;
};

struct dict_data_2 {
    QString desc;
    quint32 data_file_pos;
    quint32 data_file_len;
};

struct dictionary_header_index1 {
    quint8 size;
    quint8 dictionary_info_size;
    QByteArray dictionary_info;
    quint8 version_info_size;
    QByteArray version_info;
    quint8 build_info_size;
    QByteArray build_info;
    quint16 preindex_1_count;
    quint16 preindex_1_size;
    quint32 compindex_1_start;
    quint32 compindex_1_size;
    quint32 compindex_1_end;
    quint32 index_10kb_len;
    quint16 index_10kb_count;
    quint16 extra3;
    quint32 index_10kb_start;
    quint32 index_10kb_size;
    quint32 unk_2;
    quint32 unk_3;
    quint32 data_start;
    quint32 data_end;
    quint16 csum;
};

struct dictionary_header_index2 {
    quint8 size;
    quint8 dictionary_info_size;
    QByteArray dictionary_info;
    quint8 version_info_size;
    QByteArray version_info;
    quint8 build_info_size;
    QByteArray build_info;
    quint16 preindex_1_count;
    quint16 preindex_1_size;
    quint32 compindex_1_start;
    quint32 compindex_1_size;
    quint16 preindex_2_count;
    quint16 preindex_2_size;
    quint32 compindex_2_start;
    quint32 compindex_2_size;
    quint32 index_8b_start;
    quint32 compindex_3_size;
    quint16 index_8b_count;
    quint16 index_8b_size;
    quint32 compindex_3_start;
    quint32 compindex_3_len;
    quint32 unk;
    quint32 data_size;
    quint32 data_start;
    quint32 data_len;
    quint16 csum;
};

struct dictionary_header_math {
    quint8 size;
    quint8 dictionary_info_size;
    QByteArray dictionary_info;
    quint8 build_date_size;
    QByteArray build_date;
    quint8 build_time_size;
    QByteArray build_time;
    quint32 unk;
    quint32 header_pos;
    quint32 header_len;
    quint32 data_pos;
    quint16 csum;
};

struct dictionary_header_bible {
    quint8 size;
    quint16 dictionary_info_size;
    QByteArray dictionary_info;
    quint8 build_date_size;
    QByteArray build_date;
    quint32 index_11kb_start;
    quint32 index_11kb_len;
    quint32 data_start;
    quint32 data_len;
    quint16 csum;

};

struct powertalker_index {
    quint8 lang;
    quint32 pos, len;
};

struct powertalker_len {
    quint16 orig, tran;
};

struct powertalker_header {
    // 0: 영어, 1: 한국어, 2: 중국어, 3: 일본어
    quint8 size;
    quint16 dictionary_info_len;
    QByteArray dictionary_info;
    quint8 record_header_len;
    quint32 unk_1, unk_2, unk_3;
    struct powertalker_len lengths[4];
    struct powertalker_index index[3][4];
};

QString sort_key(QString str);
QString formatDioDictString(QString str);

struct compare_index {
public:
    bool operator()(const preindex_data& s1, const preindex_data& s2) {
        QString a = sort_key(s1.key), b = sort_key(s2.key);
        int len = qMin(a.length(), b.length());
        return QString::compare(a.left(len), b.left(len), Qt::CaseInsensitive) < 0;
    }

    bool operator()(const dict_data_2& s1, const dict_data_2& s2) {
        QString a = sort_key(s1.desc), b = sort_key(s2.desc);
        int len = qMin(a.length(), b.length());
        return QString::compare(a.left(len), b.left(len), Qt::CaseInsensitive) < 0;
    }
};

}
#endif // DEFS_H
