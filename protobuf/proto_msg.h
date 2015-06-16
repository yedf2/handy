#pragma once
#include <google/protobuf/message.h>
#include <handy/conn.h>
#include <map>

namespace handy {

typedef ::google::protobuf::Message Message;
typedef ::google::protobuf::Descriptor Descriptor;
typedef std::function<void(TcpConnPtr con, Message* msg)> ProtoCallBack;

struct ProtoMsgCodec {
    static void encode(Message* msg, Buffer& buf);
    static Message* decode(Buffer& buf);
    static bool msgComplete(Buffer& buf);
};

struct ProtoMsgDispatcher {
    void handle(TcpConnPtr con, Message* msg);
    template<typename M> void onMsg(std::function<void(TcpConnPtr con, M* msg)> cb) {
        protocbs_[M::descriptor()] = [cb](TcpConnPtr con, Message* msg) {
            cb(con, dynamic_cast<M*>( msg));
        };
    }
private:
    std::map<const Descriptor*, ProtoCallBack> protocbs_;
};

inline bool ProtoMsgCodec::msgComplete(Buffer& buf) {
    return buf.size() >= 4 && buf.size() >= *(uint32_t*)buf.begin();
}

}
