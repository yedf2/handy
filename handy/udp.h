#pragma once

#include "event_base.h"

namespace handy {

    struct UdpServer;
    struct UdpConn;
    typedef std::shared_ptr<UdpConn> UdpConnPtr;
    typedef std::shared_ptr<UdpServer> UdpServerPtr;
    typedef std::function<void(const UdpConnPtr&, Buffer)> UdpCallBack;
    typedef std::function<void(const UdpServerPtr&, Buffer, Ip4Addr)> UdpSvrCallBack;
    const int kUdpPacketSize = 4096;
    //Udp服务器
    struct UdpServer : public std::enable_shared_from_this<UdpServer>, private noncopyable {
        UdpServer(EventBases* bases);
        //return 0 on sucess, errno on error
        int bind(const std::string& host, short port, bool reusePort=false);
        static UdpServerPtr startServer(EventBases* bases, const std::string& host, short port, bool reusePort=false);
        ~UdpServer() { delete channel_; }
        Ip4Addr getAddr() { return addr_; }
        EventBase* getBase() { return base_; }
        void sendTo(Buffer msg, Ip4Addr addr) { sendTo(msg.data(), msg.size(), addr); msg.clear(); }
        void sendTo(const char* buf, size_t len, Ip4Addr addr);
        void sendTo(const std::string& s, Ip4Addr addr) { sendTo(s.data(), s.size(), addr); }
        void sendTo(const char* s, Ip4Addr addr) { sendTo(s, strlen(s), addr); }

        //消息的处理
        void onMsg(const UdpSvrCallBack& cb) { msgcb_ = cb; }
    private:
        EventBase* base_;
        EventBases* bases_;
        Ip4Addr addr_;
        Channel*channel_;
        UdpSvrCallBack msgcb_;
    };

    //Udp连接，使用引用计数
    struct UdpConn: public std::enable_shared_from_this<UdpConn>, private noncopyable {
        //Udp构造函数，实际可用的连接应当通过createConnection创建
        UdpConn(){};
        virtual ~UdpConn() {close();};
        static UdpConnPtr createConnection(EventBase* base, const std::string& host, short port);
        //automatically managed context. allocated when first used, deleted when destruct
        template<class T> T& context() { return ctx_.context<T>(); }

        EventBase* getBase() { return base_; }
        Channel* getChannel() { return channel_; }

        //发送数据
        void send(Buffer msg) { send(msg.data(), msg.size()); msg.clear(); }
        void send(const char* buf, size_t len);
        void send(const std::string& s) { send(s.data(), s.size()); }
        void send(const char* s) { send(s, strlen(s)); }
        void onMsg(const UdpCallBack& cb) { cb_ = cb; }
        void close();
        //远程地址的字符串
        std::string str() { return peer_.toString(); }
    public:
        void handleRead(const UdpConnPtr&);
        EventBase* base_;
        Channel* channel_;
        Ip4Addr local_, peer_;
        AutoContext ctx_;
        std::string destHost_;
        int destPort_;
        UdpCallBack cb_;
    };

    typedef std::function<std::string (const UdpServerPtr&, const std::string&, Ip4Addr)> RetMsgUdpCallBack;
    //半同步半异步服务器
    struct HSHAU;
    typedef std::shared_ptr<HSHAU> HSHAUPtr;
    struct HSHAU {
        static HSHAUPtr startServer(EventBase* base, const std::string& host, short port, int threads);
        HSHAU(int threads): threadPool_(threads) {}
        void exit() {threadPool_.exit(); threadPool_.join(); }
        void onMsg(const RetMsgUdpCallBack& cb);
        UdpServerPtr server_;
        ThreadPool threadPool_;
    };


}