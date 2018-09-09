#pragma once
#include "net.h"
#include "slice.h"
namespace handy {

struct CodecBase {
    // > 0 Parse the complete message, put the message in msg, return the number of bytes scanned
    // == 0 Parsing part of the message
    // < 0 Parsing error
    virtual int tryDecode(Slice data, Slice &msg) = 0;
    virtual void encode(Slice msg, Buffer &buf) = 0;
    virtual CodecBase *clone() = 0;
};

//Message ending in \r\n
struct LineCodec : public CodecBase {
    int tryDecode(Slice data, Slice &msg) override;
    void encode(Slice msg, Buffer &buf) override;
    CodecBase *clone() override { return new LineCodec(); }
};

//Give length message
struct LengthCodec : public CodecBase {
    int tryDecode(Slice data, Slice &msg) override;
    void encode(Slice msg, Buffer &buf) override;
    CodecBase *clone() override { return new LengthCodec(); }
};

};  // namespace handy
