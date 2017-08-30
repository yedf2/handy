#pragma once

#include "slice.h"
#include "conn.h"
#include <map>

namespace handy {

//base class for HttpRequest and HttpResponse
struct HttpMsg {
    enum Result{ Error, Complete, NotComplete, Continue100, };
    HttpMsg() { HttpMsg::clear(); };

    //内容添加到buf，返回写入的字节数
    virtual int encode(Buffer& buf)=0;
    //尝试从buf中解析，默认复制body内容
    virtual Result tryDecode(Slice buf, bool copyBody=true)=0;
    //清空消息相关的字段
    virtual void clear();

    std::map<std::string, std::string> headers;
    std::string version, body;
    //body可能较大，为了避免数据复制，加入body2
    Slice body2;

    std::string getHeader(const std::string& n) { return getValueFromMap_(headers, n); }
    Slice getBody() { return body2.size() ? body2 : (Slice)body; }

    //如果tryDecode返回Complete，则返回已解析的字节数
    int getByte() { return scanned_; }
protected:
    bool complete_;
    size_t contentLen_;
    size_t scanned_;
    Result tryDecode_(Slice buf, bool copyBody, Slice* line1);
    std::string getValueFromMap_(std::map<std::string, std::string>& m, const std::string& n);
};

struct HttpRequest: public HttpMsg {
    HttpRequest() { clear(); }
    std::map<std::string, std::string> args;
    std::string method, uri, query_uri;
    std::string getArg(const std::string& n) { return getValueFromMap_(args, n); }

    //override
    virtual int encode(Buffer& buf);
    virtual Result tryDecode(Slice buf, bool copyBody=true);
    virtual void clear() { HttpMsg::clear(); args.clear(); method = "GET"; query_uri = uri = ""; }
};

struct HttpResponse: public HttpMsg {
    HttpResponse() { clear(); }
    std::string statusWord;
    int status;
    void setNotFound() { setStatus(404, "Not Found"); }
    void setStatus(int st, const std::string& msg="") { status = st; statusWord = msg; body = msg; }

    //override
    virtual int encode(Buffer& buf);
    virtual Result tryDecode(Slice buf, bool copyBody=true);
    virtual void clear() { HttpMsg::clear(); status = 200; statusWord = "OK"; }
};

//Http连接本质上是一条Tcp连接，下面的封装主要是加入了HttpRequest，HttpResponse的处理
struct HttpConnPtr {
    TcpConnPtr tcp;
    HttpConnPtr(const TcpConnPtr& con):tcp(con) {}
    operator TcpConnPtr() const { return tcp; }
    TcpConn* operator ->() const { return tcp.get(); }
    bool operator < (const HttpConnPtr& con) const { return tcp < con.tcp; }

    typedef std::function<void(const HttpConnPtr&)> HttpCallBack;

    HttpRequest& getRequest() const { return tcp->internalCtx_.context<HttpContext>().req; }
    HttpResponse& getResponse() const { return tcp->internalCtx_.context<HttpContext>().resp; }

    void sendRequest() const { sendRequest(getRequest()); }
    void sendResponse() const { sendResponse(getResponse()); }
    void sendRequest(HttpRequest& req) const { req.encode(tcp->getOutput()); logOutput("http req"); clearData(); tcp->sendOutput(); }
    void sendResponse(HttpResponse& resp) const { resp.encode(tcp->getOutput()); logOutput("http resp"); clearData(); tcp->sendOutput(); }
    //文件作为Response
    void sendFile(const std::string& filename) const;
    void clearData() const;

    void onHttpMsg(const HttpCallBack& cb) const;
protected:
    struct HttpContext {
        HttpRequest req;
        HttpResponse resp;
    };
    void handleRead(const HttpCallBack& cb) const;
    void logOutput(const char* title) const;
};

typedef HttpConnPtr::HttpCallBack HttpCallBack;

//http服务器
struct HttpServer: public TcpServer {
    HttpServer(EventBases* base);
    template <class Conn=TcpConn> void setConnType() { conncb_ = []{ return TcpConnPtr(new Conn); }; }
    void onGet(const std::string& uri, const HttpCallBack& cb) { cbs_["GET"][uri] = cb; }
    void onRequest(const std::string& method, const std::string& uri, const HttpCallBack& cb) { cbs_[method][uri] = cb; }
    void onDefault(const HttpCallBack& cb) { defcb_ = cb; }
private:
    HttpCallBack defcb_;
    std::function<TcpConnPtr()> conncb_;
    std::map<std::string, std::map<std::string, HttpCallBack>> cbs_;
};

}
