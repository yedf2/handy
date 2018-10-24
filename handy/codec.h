#pragma once
#include "slice.h"
#include "net.h"
namespace handy {

struct CodecBase {
    //If `data.size()` > 0, then prase the complete message, put the message in msg, return the number of bytes scanned.
    //If `data.size()` == 0, then prase part of message.
    //If `data.size()` < 0, then prase error.
    virtual int tryDecode(Slice data, Slice& msg) = 0;
    virtual void encode(Slice msg, Buffer& buf) = 0;
    virtual CodecBase* clone() = 0;
};

//The message which ending in \r\n.
struct LineCodec: public CodecBase{
    int tryDecode(Slice data, Slice& msg) override;
    void encode(Slice msg, Buffer& buf) override;
    CodecBase* clone() override { return new LineCodec(); }
};

//The message which gives the length.
struct LengthCodec:public CodecBase {
    int tryDecode(Slice data, Slice& msg) override;
    void encode(Slice msg, Buffer& buf) override;
    CodecBase* clone() override { return new LengthCodec(); }
};

};
