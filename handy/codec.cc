#include "codec.h"

using namespace std;

namespace handy {

int LineCodec::tryDecode(Slice data, Slice& msg) {
    for (size_t i = 0; i < data.size()-1; i ++) {
        if (data[i] == '\r' && data[i+1] == '\n') {
            msg = Slice(data.data(), i);
            return i+2;
        }
    }
    return 0;
}
void LineCodec::encode(Slice msg, Buffer& buf) {
    buf.append(msg).append("\r\n");
}

int LengthCodec::tryDecode(Slice data, Slice& msg) {
    if (data.size() < 8) {
        return 0;
    }
    int len = net::ntoh(*(int32_t*)(data.data()+4));
    if (len > 1024*1024 || memcmp(data.data(), "mBdT", 4) != 0) {
        return -1;
    }
    if ((int)data.size() >= len+8) {
        msg = Slice(data.data()+8, len);
        return len+8;
    }
    return 0;
}
void LengthCodec::encode(Slice msg, Buffer& buf) {
    buf.append("mBdT").appendValue(net::hton((int32_t)msg.size())).append(msg);
}


}