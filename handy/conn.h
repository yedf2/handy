#pragma once
#include "event_base.h"

namespace handy {

	//Tcp connection which uses reference counting.
    struct TcpConn: public std::enable_shared_from_this<TcpConn> {
        //Status of the `TcpConn`.
        enum State { Invalid=1, Handshaking, Connected, Closed, Failed, };
        //`TcpConn` constructor, the actual available connection should be created via createConnection().
        TcpConn();
        virtual ~TcpConn();
        //The function can pass in the connection type and return the smart pointer.
        template<class C=TcpConn> static TcpConnPtr createConnection(EventBase* base, const std::string& host, short port,
                                   int timeout=0, const std::string& localip="") {
            TcpConnPtr con(new C); con->isClient_ = true; return con->connect(base, host, port, timeout, localip) ? NULL : con;
        }
        template<class C=TcpConn> static TcpConnPtr createConnection(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer) {
            TcpConnPtr con(new C); con->attach(base, fd, local, peer); return con;
        }

        bool isClient() { return isClient_; }
        //automatically managed context. allocated when first used, deleted when destruct
        template<class T> T& context() { return ctx_.context<T>(); }

        EventBase* getBase() { return channel_ ? channel_->getBase() : NULL; }
        State getState() { return state_; }
        //`Buffer` of `TcpConn` input and output.
        Buffer& getInput() { return input_; }
        Buffer& getOutput() { return output_; }

        Channel* getChannel() { return channel_; }
        bool writable() { return channel_ ? channel_->writeEnabled(): false; }

        //Send data.
        void sendOutput() { send(output_); }
        void send(Buffer& msg);
        void send(const char* buf, size_t len);
        void send(const std::string& s) { send(s.data(), s.size()); }
        void send(const char* s) { send(s, strlen(s)); }

        //Callback when data arrives.
        void onRead(const TcpCallBack& cb) { assert(!readcb_); readcb_ = cb; };
        //Callback when the buffer of the object of `TcpConn` is writable.
        void onWritable(const TcpCallBack& cb) { writablecb_ = cb;}
        //Callback when the status of the object of `TcpConn` changes.
        void onState(const TcpCallBack& cb) { statecb_ = cb; }
        //Callback if tcp connection is idle.
        void addIdleCB(int idle, const TcpCallBack& cb);

        //Message callback, this callback conflicts with the `onRead()` callback, only one can be called.
        //The ownership of codec is given to `onMsg()`.
        void onMsg(CodecBase* codec, const MsgCallBack& cb);
        //Send message.
        void sendMsg(Slice msg);

        //`TcpConn` will be processed in the next event cycle.
        void close();

        //!Use with caution.Closing the connection immediately, cleaning up the related resources, may cause the reference count of the connection to become 0, so that the connection referenced by the current caller is destructed.
        void closeNow() { if (channel_) channel_->close(); }

        //String about remote address.
        std::string str() { return peer_.toString(); }
    public:
        Channel* channel_;
        Buffer input_, output_;
        Ip4Addr local_, peer_;
        State state_;
        TcpCallBack readcb_, writablecb_, statecb_;
        std::list<IdleId> idleIds_;
        AutoContext ctx_, internalCtx_;
        bool isClient_;
        std::unique_ptr<CodecBase> codec_;
        void handleRead(const TcpConnPtr& con);
        void handleWrite(const TcpConnPtr& con);
        ssize_t isend(const char* buf, size_t len);
        void cleanup(const TcpConnPtr& con);
        int connect(EventBase* base, const std::string& host, short port, int timeout, const std::string& localip);
        void attach(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer);
        virtual int readImp(int fd, void* buf, size_t bytes) { return ::read(fd, buf, bytes); }
        virtual int writeImp(int fd, const void* buf, size_t bytes) { return ::write(fd, buf, bytes); }
        virtual int handleHandshake(const TcpConnPtr& con);
    };

    //Tcp server.
    struct TcpServer {
        TcpServer(EventBases* bases);
        //return 0 on sucess, errno on error
        int bind(const std::string& host, short port);
        static TcpServerPtr startServer(EventBases* bases, const std::string& host, short port);
        ~TcpServer() { delete listen_channel_; }
        Ip4Addr getAddr() { return addr_; }
        EventBase* getBase() { return base_; }
        void onConnCreate(const std::function<TcpConnPtr()>& cb) { createcb_ = cb; }
        void onConnRead(const TcpCallBack& cb) { readcb_ = cb; assert(!msgcb_); }
        //Message processing conflicts with `Read()` callback, only one can be called.
        void onConnMsg(CodecBase* codec, const MsgCallBack& cb) { codec_.reset(codec); msgcb_ = cb; assert(!readcb_); }
    private:
        EventBase* base_;
        EventBases* bases_;
        Ip4Addr addr_;
        Channel* listen_channel_;
        TcpCallBack readcb_;
        MsgCallBack msgcb_;
        std::function<TcpConnPtr()> createcb_;
        std::unique_ptr<CodecBase> codec_;
        void handleAccept();
    };

    typedef std::function<std::string (const TcpConnPtr&, const std::string& msg)> RetMsgCallBack;
    //Half synchronous and half asynchronous server.
    struct HSHA;
    typedef std::shared_ptr<HSHA> HSHAPtr;
    struct HSHA {
        static HSHAPtr startServer(EventBase* base, const std::string& host, short port, int threads);
        HSHA(int threads): threadPool_(threads) {}
        void exit() {threadPool_.exit(); threadPool_.join(); }
        void onMsg(CodecBase* codec, const RetMsgCallBack& cb);
        TcpServerPtr server_;
        ThreadPool threadPool_;
    };


}
