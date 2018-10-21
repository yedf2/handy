#pragma once
#include "slice.h"
#include "net.h"
namespace handy {

struct CodecBase {
    // > 0 prase the complete message, put the message in msg, return the number of bytes scanned
    // == 0 prase part of message
    // < 0 prase error
    virtual int tryDecode(Slice data, Slice& msg) = 0;
    virtual void encode(Slice msg, Buffer& buf) = 0;
    virtual CodecBase* clone() = 0;
};

//message ending in \r\n
struct LineCodec: public CodecBase{
    int tryDecode(Slice data, Slice& msg) override;
    void encode(Slice msg, Buffer& buf) override;
    CodecBase* clone() override { return new LineCodec(); }
};

//the message gives the length
struct LengthCodec:public CodecBase {
    int tryDecode(Slice data, Slice& msg) override;
    void encode(Slice msg, Buffer& buf) override;
    CodecBase* clone() override { return new LengthCodec(); }
};

};
