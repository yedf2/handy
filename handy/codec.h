#pragma once
#include "slice.h"
#include "net.h"
namespace handy {

struct CodecBase {
    // > 0 解析出完整消息，消息放在msg中，返回已扫描的字节数
    // == 0 解析部分消息
    // < 0 解析错误
    virtual int tryDecode(Slice data, Slice& msg) = 0;
    virtual void encode(Slice msg, Buffer& buf) = 0;
};

//以换行结尾的消息
struct LineCodec: public CodecBase{
    virtual int tryDecode(Slice data, Slice& msg) {
        for (size_t i = 0; i < data.size(); i ++) {
            if (data[i] == '\n') {
                msg = Slice(data.data(), i);
                return i+1;
            }
        }
        return 0;
    }
    virtual void encode(Slice msg, Buffer& buf) {
        buf.append(msg).appendValue('\n');
    }
};

//给出长度的消息
struct LengthCodec:public CodecBase {
    virtual int tryDecode(Slice data, Slice& msg) {
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
    virtual void encode(Slice msg, Buffer& buf) {
        buf.append("mBdT").appendValue(net::hton((int32_t)msg.size())).append(msg);
    }
};

};
