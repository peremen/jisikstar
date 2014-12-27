#ifndef MATHDICTREADER_H
#define MATHDICTREADER_H

#include <QObject>
#include <defs.h>

class MathDictReader : public QObject {
    Q_OBJECT

private:
    QString filename;
    bool is_loaded;
    struct Snotra::dict_data *root_data;
    Snotra::dictionary_header_math header_data;
    void clear_root_data();
    QPixmap get_data(const QModelIndex &index) const;

public:
    explicit MathDictReader(QObject *parent = 0);
    MathDictReader(const QString &filename, QObject *parent = 0);
    ~MathDictReader();

    bool load(const QString &filename);
    bool isLoaded();
    bool exportToFile(const QString &destination, Snotra::FileType type);

signals:

public slots:

};

#endif // MATHDICTMODEL_H
