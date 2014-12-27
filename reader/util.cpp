#include <QString>
#include <QRegularExpression>
#include <defs.h>
#include <QDebug>

namespace Snotra {
    QString sort_key(QString str) {
        return str.repeated(1).remove(QString::fromUtf8("ː")).remove(":") \
               .remove("-").remove(QString::fromUtf8("－")).remove(QString::fromUtf8("⌒"));
    }

    QString formatDioDictString(QString str) {
        // %(F|K|P|M)
        // %(B|b|I|i|T|t|C[0-9]|c[0-9]|D|d|E|e|Z[0-9]|z[0-9])
        // %(H|h|X|x|O|o|Y|y|W[0-9]|w[0-9]|Q|q)

        // %F: Level marker
        // %K: Keyword
        // %P: Pronounciation
        // %M: Meaning (line break)
        // Order: %F - %K - %P - %M
        // Grouped till next percent/%M or linebreak
        QRegularExpression head("(%F|%K|%P|%M)");
        QRegularExpressionMatchIterator i = head.globalMatch(str);
        bool headerFlags[4] = {false, false, false, false};

        while (i.hasNext()) {
            QRegularExpressionMatch m = i.next();
            if (m.captured(1) == "%F") {
                str.replace("%F", "<font color=\"#e28000\">");
                headerFlags[0] = true;
            } else if (m.captured(1) == "%K") {
                if (headerFlags[0]) {
                    str.replace("%K", "</font><font color=\"#277ce5\">");
                } else {
                    str.replace("%K", "<font color=\"#277ce5\">");
                }
                headerFlags[1] = true;
            } else if (m.captured(1) == "%P") {
                if (headerFlags[0] | headerFlags[1]) {
                    str.replace("%P", "</font><font color=\"#505050\">");
                } else {
                    str.replace("%P", "<font color=\"#505050\">");
                }
                headerFlags[2] = true;
            } else if (m.captured(1) == "%M") {
                if (headerFlags[0] | headerFlags[1] | headerFlags[2]) {
                    str.replace("%M", "</font></big><br>");
                } else {
                    str.replace("%M", "</big><br>");
                }
                headerFlags[3] = true;
            } else {
                Q_ASSERT(false);
            }
        }
        str.prepend(QString::fromLatin1("<big>"));

        // %B-%b: Bold
        str.replace("%B", "<b>");
        str.replace("%b", "</b>");

        // %I-%i: Italic
        str.replace("%I", "<i>");
        str.replace("%i", "</i>");

        // %T-%t: Article
        str.replace("%T", "<font color=\"#3104b4\">");
        str.replace("%t", "</font>");

        // %C[0-9]-%c[0-9]: Disambiguation
        str.replace(QRegularExpression("%C[0-9]"), "<font color=\"#e28000\">");
        str.replace(QRegularExpression("%c[0-9]"), "</font>");

        // %D-%d, %E-%e: Example
        str.replace("%E", "<font color=\"#505050\">");
        str.replace("%e", "</font>");
        str.replace("%D", "<font color=\"#505050\">");
        str.replace("%d", "</font>");

        // %Z1-%z1
        str.replace("%Z1", "<font color=\"#277ce5\">");
        str.replace("%z1", "</font>");

        // %Z2-%z2
        str.replace("%Z2", "<font color=\"#505050\">");
        str.replace("%z2", "</font>");

        // %Z8-%z8
        str.replace("%Z8", "<u>");
        str.replace("%z8", "</u>");

        // %H-%h: Only in Korean-Japanese dictionary, nothing special
        str.replace("%H", "");
        str.replace("%h", "");

        // %X-%x: TOEIC tips, nothing special
        str.replace("%X", "");
        str.replace("%x", "");

        // %O-%o: English-Korean dictionary, roots, nothing special
        str.replace("%O", "");
        str.replace("%o", "");

        // %Y-%y: Synonym, nothing special
        str.replace("%Y", "");
        str.replace("%y", "");

        // %W[0-9]-%w[0-9]: alternative meaning, nothing special
        str.replace(QRegularExpression("%W[0-9]"), "");
        str.replace(QRegularExpression("%w[0-9]"), "");

        // %Q-%q: Chinese-Korean dictionary, Korean Hanja sound
        str.replace("%Q", "");
        str.replace("%q", "");

        return str;
    }
}
