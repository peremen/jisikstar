#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QFileInfo>

#include "converthandler.h"
#include "dictdbindex1reader.h"
#include "dictdbindex2reader.h"
#include "mathdictreader.h"
#include "powerbiblereader.h"
#include "powertalkerreader.h"

ConvertHandler::ConvertHandler(QObject *parent) :
    QObject(parent)
{
    this->dictFileList << "DDEngToKorDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "NewE4U_YBMEngToKorDictionaryKSCSUIDDBwH.dat";
    this->dictFileList << "NewE4U_YBMEngToKorDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "AllInAll_YBMEngToKorDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "NewACEEngToKorDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "KumsungEngToKorDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "DDKorToEngDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "NewE4U_YBMKorToEngDictionaryKSCSUIDDBwH.dat";
    this->dictFileList << "NewE4U_YBMKorToEngDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "AllInAll_YBMKorToEngDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "NewACEKorToEngDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "KumsungKorToEngDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "CollinsEngToEngDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "OxfordEngToEngDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "DDEngToKorToeicWordUCS2LESUIDDBwH.dat";
    this->dictFileList << "YBMEngToKorToeicWordKSCSUIDDBwH.dat";
    this->dictFileList << "YBMEngToKorSunungWordKSCSUIDDBwH.dat";
    this->dictFileList << "DDJpnToKorDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "DDKorToJpnDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "DDChnToKorDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "DDKorToChnDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "DDKorToKorDictionaryUCS2LESUIDDBwH.dat";
    this->dictFileList << "BritannicaKorToKorEncyclopediaUCS2LESUIDDBwH.dat";

    this->dictNameList << "English-Korean";
    this->dictNameList << "e4U English-Korean";
    this->dictNameList << "e4U English-Korean";
    this->dictNameList << "All-in-All English-Korean";
    this->dictNameList << "NewACE English-Korean";
    this->dictNameList << "Kumsung English-Korean";
    this->dictNameList << "Korean-English";
    this->dictNameList << "e4U Korean-English";
    this->dictNameList << "e4U Korean-English";
    this->dictNameList << "All-in-All Korean-English";
    this->dictNameList << "NewACE Korean-English";
    this->dictNameList << "Kumsung Korean-English";
    this->dictNameList << "Collins English Dictionary";
    this->dictNameList << "Oxford English Dictionary";
    this->dictNameList << "TOEIC Dictionary";
    this->dictNameList << "TOEIC Dictionary";
    this->dictNameList << "Suneung Dictionary";
    this->dictNameList << "Japanese-Korean";
    this->dictNameList << "Korean-Japanese";
    this->dictNameList << "Chinese-Korean";
    this->dictNameList << "Korean-Chinese";
    this->dictNameList << "Korean Dictionary";
    this->dictNameList << "Encyclopedia";

    this->mathDictFileList << "MathDictDB.dat";
    this->mathDictFileList << "PhysDictDB.dat";
    this->mathDictFileList << "ChemDictDB.dat";

    this->mathDictNameList << "Math Dictionary";
    this->mathDictNameList << "Physics Dictionary";
    this->mathDictNameList << "Chemistry Dictionary";

    this->powerBibleFileList << "PowerBible_kor.dat";
    this->powerBibleFileList << "PowerBible_eng.dat";
    this->powerBibleNameList << "Old Testament" << "New Testament";

    this->powerTalkerFileList << "PowerTalkerDB.dat";
    this->powerTalkerNameList << "English Conversation" << "Japanese Conversation" << "Chinese Conversation";

}

void ConvertHandler::setSource(QString source) {
    this->source = source;
}

void ConvertHandler::setDestination(QString destination) {
    this->destination = destination;
}

void ConvertHandler::setFileType(Snotra::FileType f) {
    this->fileType = f;
}

void ConvertHandler::convert() {
    int mode = -1;
    QString s;

    if (!QFile::exists(this->source)) {
        emit conversionFinished(1);
        return;
    }

    foreach (s, this->dictFileList) {
        if (this->source.contains(s, Qt::CaseInsensitive)) mode = 1;
    }
    foreach (s, this->mathDictFileList) {
        if (this->source.contains(s, Qt::CaseInsensitive)) mode = 2;
    }
    foreach (s, this->powerBibleFileList) {
        if (this->source.contains(s, Qt::CaseInsensitive)) mode = 3;
    }
    foreach (s, this->powerTalkerFileList) {
        if (this->source.contains(s, Qt::CaseInsensitive)) mode = 4;
    }
    if (mode < 0) {
        emit conversionFinished(2);
        return;
    }

    switch (mode) {
    case 1:
        convertDictFile(this->source, this->destination, this->fileType);
        break;
    case 2:
        convertMathDictFile(this->source, this->destination, this->fileType);
        break;
    case 3:
        convertPowerBibleFile(this->source, this->destination, this->fileType);
        break;
    case 4:
        convertPowerTalkerFile(this->source, this->destination, this->fileType);
        break;
    default:
        emit conversionFinished(2);
        return;
    }
}

void ConvertHandler::convertDictFile(QString source, QString destination, Snotra::FileType t) {
    quint8 header_len;
    QFile f;
    QFileInfo fi;
    DictDBIndex1Reader *reader1 = NULL;
    DictDBIndex2Reader *reader2 = NULL;

    fi.setFile(destination);
    f.setFileName(source);
    if (!f.open(QIODevice::ReadOnly)) {
        emit conversionFinished(1);
        return;
    }
    f.read((char *)&header_len, sizeof(header_len));
    f.close();

    switch (header_len) {
    case 0x77:
    case 0x78:
    case 0x7a:
    case 0x7b:
    case 0x7e:
    case 0x83:
        reader1 = new DictDBIndex1Reader(source);
        reader1->exportToFile(destination, t);
        break;
    case 0x91:
    case 0x95:
        reader2 = new DictDBIndex2Reader(source);
        reader2->exportToFile(QString("%1/%2_0.%3").
                              arg(fi.absoluteDir().canonicalPath()).
                              arg(fi.baseName()).
                              arg(fi.completeSuffix()), t, 0);

        reader2->exportToFile(QString("%1/%2_1.%3").
                              arg(fi.absoluteDir().canonicalPath()).
                              arg(fi.baseName()).
                              arg(fi.completeSuffix()), t, 1);
        break;
    default:
        emit conversionFinished(2);
        return;
    }

    if (reader1) delete reader1;
    if (reader2) delete reader2;
    emit conversionFinished(0);
}

void ConvertHandler::convertMathDictFile(QString source, QString destination, Snotra::FileType f) {
    MathDictReader *reader;

    reader = new MathDictReader(source);
    reader->exportToFile(destination, f);
    delete reader;

    emit conversionFinished(0);
}

void ConvertHandler::convertPowerBibleFile(QString source, QString destination, Snotra::FileType f) {
    PowerBibleReader *reader;
    QFileInfo fi(destination);

    reader = new PowerBibleReader(source, Snotra::NewTestament);
    reader->exportToFile(QString("%1/%2_new.%3").
                         arg(fi.absoluteDir().canonicalPath()).
                         arg(fi.baseName()).
                         arg(fi.completeSuffix()), f);
    delete reader;

    reader = new PowerBibleReader(source, Snotra::OldTestament);
    reader->exportToFile(QString("%1/%2_old.%3").
                         arg(fi.absoluteDir().canonicalPath()).
                         arg(fi.baseName()).
                         arg(fi.completeSuffix()), f);
    delete reader;

    emit conversionFinished(0);
}

void ConvertHandler::convertPowerTalkerFile(QString source, QString destination, Snotra::FileType f) {
    PowerTalkerReader *reader;

    /*
    reader = new PowerTalkerReader(source);
    delete reader;
    */
    Q_UNUSED(reader);
}
