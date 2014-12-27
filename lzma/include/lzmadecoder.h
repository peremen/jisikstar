#ifndef LZMADECODER_H
#define LZMADECODER_H

#include "LzmaDec.h"
#include <QtGlobal>

class LZMADecoder
{
public:
    LZMADecoder();
    static bool decodeLZMA(char *in, quint64 inSize,
                           char *out, quint64 outSize);
    static bool decodeLZMA(const QByteArray& in, QByteArray& out);
};

#endif // LZMADECODER_H
