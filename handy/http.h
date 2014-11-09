#pragma once

#include "slice.h"
#include "handy.h"
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

    std::string getHeader(const std::string& n) { return map_get(headers, n); }
    Slice getBody() { return body2.size() ? body2 : (Slice)body; }

    //如果tryDecode返回Complete，则返回已解析的字节数
    int getByte() { return scanned_; }
protected:
    bool complete_;
    size_t contentLen_;
    size_t scanned_;
    Result tryDecode_(Slice buf, bool copyBody, Slice* line1);
    std::string map_get(std::map<std::string, std::string>& m, const std::string& n);
};

struct HttpRequest: public HttpMsg {
    HttpRequest() { clear(); }
    std::map<std::string, std::string> args;
    std::string method, uri, query_uri;
    std::string getArg(const std::string& n) { return map_get(args, n); }

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
struct HttpConn;

//Http连接的智能指针，提供了TcpConn到HttpConn直接的转换
struct HttpConnPtr {
    TcpConnPtr tcp;
    HttpConnPtr(HttpConn* hcon): tcp(hcon) {}
    HttpConnPtr(const TcpConnPtr& con):tcp(con) {}
    operator TcpConnPtr() const { return tcp; }
    HttpConn* operator ->() const { return (HttpConn*)tcp.get(); }
    bool operator < (const HttpConnPtr& con) const { return tcp < con.tcp; }
};

struct HttpConn: public TcpConn {
    typedef std::function<void(const HttpConnPtr&)> HttpCallBack;
    int connect(EventBase* base, const std::string& host, short port, int timeout=0) { return TcpConn::connect(base, host, port, timeout); }

    HttpRequest& getRequest() { return req_; }
    HttpResponse& getResponse() { return resp_; }

    void sendRequest() { sendRequest(getRequest()); }
    void sendResponse() { sendResponse(getResponse()); }
    void sendRequest(HttpRequest& req) { req.encode(getOutput()); logOutput("http req"); clearData(); sendOutput(); }
    void sendResponse(HttpResponse& resp) { resp.encode(getOutput()); logOutput("http resp"); clearData(); sendOutput(); }
    //文件作为Response
    void sendFile(const std::string& filename);
    void clearData();

    void onMsg(const HttpCallBack& cb);
protected:
    HttpRequest req_;
    HttpResponse resp_;
    void handleRead(const HttpCallBack& cb);
    void logOutput(const char* title);
};

typedef HttpConn::HttpCallBack HttpCallBack;

//http服务器
struct HttpServer {
    HttpServer(EventBases* base, const std::string& host, short port);
    void onGet(const std::string& uri, const HttpCallBack& cb) { cbs_["GET"][uri] = cb; }
    void onRequest(const std::string& method, const std::string& uri, const HttpCallBack& cb) { cbs_[method][uri] = cb; }
    void onDefault(const HttpCallBack& cb) { defcb_ = cb; }
private:
    TcpServer server_;
    HttpCallBack defcb_;
    std::map<std::string, std::map<std::string, HttpCallBack>> cbs_;
};

}
