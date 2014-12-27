#ifndef CONVERTHANDLER_H
#define CONVERTHANDLER_H

#include <QObject>
#include <QFile>
#include <QStringList>
#include "defs.h"

class ConvertHandler : public QObject
{
    Q_OBJECT
public:
    explicit ConvertHandler(QObject *parent = 0);
    void setSource(QString source);
    void setDestination(QString destination);
    void setFileType(Snotra::FileType f);

signals:
    void conversionFinished(int status);

public slots:
    void convert();

private:
    QStringList dictFileList, dictNameList;
    QStringList mathDictFileList, mathDictNameList;
    QStringList powerBibleFileList, powerBibleNameList;
    QStringList powerTalkerFileList, powerTalkerNameList;

    QString source, destination;
    Snotra::FileType fileType;

    void convertDictFile(QString source, QString destination, Snotra::FileType f);
    void convertMathDictFile(QString source, QString destination, Snotra::FileType f);
    void convertPowerBibleFile(QString source, QString destination, Snotra::FileType f);
    void convertPowerTalkerFile(QString source, QString destination, Snotra::FileType f);

};

#endif // CONVERTHANDLER_H
