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

//以\r\n结尾的消息
struct LineCodec: public CodecBase{
    virtual int tryDecode(Slice data, Slice& msg);
    virtual void encode(Slice msg, Buffer& buf);
};

//给出长度的消息
struct LengthCodec:public CodecBase {
    virtual int tryDecode(Slice data, Slice& msg);
    virtual void encode(Slice msg, Buffer& buf);
};

};
