#include "proto_msg.h"
#include <string>
#include <google/protobuf/descriptor.h>

using namespace std;
using namespace google::protobuf;

namespace handy {
void ProtoMsgDispatcher::handle(TcpConnPtr con, Message* msg) {
    auto p = protocbs_.find(msg->GetDescriptor());
    if (p != protocbs_.end()) {
        p->second(con, msg);
    } else {
        error("unknown message type %s", msg->GetTypeName().c_str());
    }
}

// 4 byte total msg len, including this 4 bytes
// 4 byte name len
// name string not null ended
// protobuf data
Message* ProtoMsgCodec::decode(Buffer& s){
    if (s.size() < 8) {
        error("buffer is too small size: %lu", s.size());
        return NULL;
    }
    char* p = s.data();
    uint32_t msglen = *(uint32_t*)p;
    uint32_t namelen = *(uint32_t*)(p+4);
    if (s.size() < msglen || s.size() < 4 + namelen) {
        error("buf format error size %lu msglen %d namelen %d", 
            s.size(), msglen, namelen);
        return NULL;
    }
    string typeName(p + 8, namelen);
    Message* msg = NULL;
    const Descriptor* des = DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
    if (des) {
        const Message* proto = MessageFactory::generated_factory()->GetPrototype(des);
        if (proto) {
            msg = proto->New();
        }
    }
    if (msg == NULL) {
        error("cannot create Message for %s", typeName.c_str());
        return NULL;
    }
    int r = msg->ParseFromArray(p + 8 + namelen, msglen - 8 - namelen);
    if (!r) {
        error("bad msg for protobuf");
        delete msg;
        return NULL;
    }
    s.consume(msglen);
    return msg;
}

void ProtoMsgCodec::encode(Message* msg, Buffer& buf) {
    size_t offset = buf.size();
    buf.appendValue((uint32_t)0);
    const string& typeName = msg->GetDescriptor()->full_name();
    buf.appendValue((uint32_t)typeName.size());
    buf.append(typeName.data(), typeName.size());
    msg->SerializeToArray(buf.allocRoom(msg->ByteSize()), msg->ByteSize());
    *(uint32_t*)(buf.begin()+offset) = buf.size() - offset;
}

}
