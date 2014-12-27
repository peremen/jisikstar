#include <QString>
#include <defs.h>

namespace Snotra {
    QString sort_key(QString str) {
        return str.repeated(1).remove(QString::fromUtf8("ː")).remove(":") \
               .remove("-").remove(QString::fromUtf8("－")).remove(QString::fromUtf8("⌒"));
    }
}
