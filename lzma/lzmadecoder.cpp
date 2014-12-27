#include "lzmadecoder.h"
#include "LzmaLib.h"
#include <QByteArray>

LZMADecoder::LZMADecoder(){

}

bool LZMADecoder::decodeLZMA(char *in, quint64 inSize,
                             char *out, quint64 outSize) {
    quint64 out_size = outSize;
    size_t in_done = inSize - (LZMA_PROPS_SIZE+8);
    size_t out_done = out_size;
    int ret = LzmaUncompress((unsigned char *)out, &out_done,
                             (unsigned char *)in + (LZMA_PROPS_SIZE + 8),
                             &in_done,
                             (unsigned char *)in, LZMA_PROPS_SIZE);
    if (ret == SZ_OK) {
        return true;
    } else {
        return false;
    }
}

bool LZMADecoder::decodeLZMA(const QByteArray& in, QByteArray& out) {
    quint64 out_size = *(quint64 *)in.mid(5, sizeof(quint64)).data();
    out.resize(out_size);
    unsigned char *buf_in = (unsigned char *)in.data(), *buf_out = (unsigned char *)out.data();
    size_t in_done = in.size() - (LZMA_PROPS_SIZE+8), out_done = out_size;
    int ret = LzmaUncompress(buf_out, &out_done, buf_in+(LZMA_PROPS_SIZE+8), &in_done, buf_in, LZMA_PROPS_SIZE);
    if(ret == SZ_OK) {
        out.resize(out_done);
        return true;
    } else {
        return false;
    }
}
