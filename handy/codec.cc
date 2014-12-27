#include "codec.h"

using namespace std;

namespace handy {

int LineCodec::tryDecode(Slice data, Slice& msg) {
    if (data.size() == 1 && data[0] == 0x04) {
        msg = data;
        return 1;
    }
    for (size_t i = 0; i < data.size(); i ++) {
        if (data[i] == '\n') {
            if (i > 0 && data[i-1] == '\r') {
                msg = Slice(data.data(), i-1);
                return i+1;
            } else {
                msg = Slice(data.data(), i);
                return i+1;
            }
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