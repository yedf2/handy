#pragma once

#include "slice.h"
#include "handy.h"
#include <map>

namespace handy {

//base class for HttpRequest and HttpResponse
struct HttpMsg {
    enum Result{ Error, Complete, NotComplete, Continue100, };
    HttpMsg() { clear_(); };
    std::string getHeader(const std::string& n) { return map_get(headers, n); }
    Slice getBody() { return body2.size() ? body2 : (Slice)body; }
    std::map<std::string, std::string> headers;
    std::string version, body;
    Slice body2;
    //return how many byte has been decoded if tryDecode returns Complete
    int getByte() { return scanned_; }
protected:
    bool complete_;
    size_t contentLen_;
    size_t scanned_;
    void clear_();
    Result tryDecode_(Slice buf, bool copyBody, Slice* line1);
    std::string map_get(std::map<std::string, std::string>& m, const std::string& n);
};

struct HttpRequest: public HttpMsg {
    HttpRequest() { clear(); }
    //return how many byte has been encoded
    int encode(Buffer& buf);
    Result tryDecode(Slice buf, bool copyBody=true);
    void clear() { clear_(); args.clear(); method = "GET"; query_uri = uri = ""; }

    std::string getArg(const std::string& n) { return map_get(args, n); }
    std::map<std::string, std::string> args;
    std::string method, uri, query_uri;
};

struct HttpResponse: public HttpMsg {
    HttpResponse() { clear(); }
    //return how many byte has been encoded
    int encode(Buffer& buf);
    Result tryDecode(Slice buf, bool copyBody=true);
    void clear() { status = 200; statusWord = "OK"; clear_(); }

    void setNotFound() { body2 = Slice("Not Found"); setStatus(404, "Not Found"); }
    void setStatus(int st, const std::string& msg="") { status = st; statusWord = msg; body = msg; }
    std::string statusWord;
    int status;
};

class HttpConn;

struct HttpConnPtr {
    TcpConnPtr tcp;
    static HttpConn* rawHttp(const TcpConnPtr& con) { return (HttpConn*)con.get(); }
    HttpConnPtr(const TcpConnPtr& con):tcp(con) {}
    operator TcpConnPtr() const { return tcp; }
    HttpConn* operator ->() const { return (HttpConn*)tcp.get(); }
    bool operator < (const HttpConnPtr& con) const { return tcp < con.tcp; }
};

struct HttpConn: public TcpConn {
    typedef std::function<void(const HttpConnPtr&)> HttpCallBack;
    static HttpConnPtr connectTo(EventBase* base, Ip4Addr addr, int timeout=0);
    static HttpConnPtr connectTo(EventBase* base, const std::string& host, short port, int timeout=0) { return connectTo(base, Ip4Addr(host, port), timeout); }

    HttpRequest& getRequest() { return hctx().req; }
    HttpResponse& getResponse() { return hctx().resp; }

    void sendRequest() { sendRequest(getRequest()); }
    void sendResponse() { sendResponse(getResponse()); }
    void sendRequest(HttpRequest& req) { req.encode(getOutput()); clearData(); sendOutput(); }
    void sendResponse(HttpResponse& resp) { resp.encode(getOutput()); clearData(); sendOutput(); }
    void sendFile(const std::string& filename);
    void clearData();

    void onMsg(const HttpCallBack& cb);
    void setType(bool isClient) { hctx().type = isClient ? Client : Server; }
protected:
    enum Type{ Unknown=0, Client, Server, };
    struct HttpContext{
        HttpRequest req;
        HttpResponse resp;
        Type type;
        HttpContext(): type(Unknown){}
    };
    HttpContext& hctx() { return internalCtx_.context<HttpContext>(); }
    void handleRead(const HttpCallBack& cb);
};

typedef HttpConn::HttpCallBack HttpCallBack;

struct HttpServer {
    HttpServer(EventBase* base, Ip4Addr addr):server_(base, addr) { init(); }
    HttpServer(EventBase* base, const std::string& host, int port):HttpServer(base, Ip4Addr(host, port)) {};
    HttpServer(MultiBase* base, Ip4Addr addr):server_(base, addr) { init(); }
    HttpServer(MultiBase* base, const std::string& host, int port):HttpServer(base, Ip4Addr(host, port)) {};
    void onGet(const std::string& uri, const HttpCallBack& cb) { cbs_["GET"][uri] = cb; }
    void onRequest(const std::string& method, const std::string& uri, const HttpCallBack& cb) { cbs_[method][uri] = cb; }
    void onDefault(const HttpCallBack& cb) { defcb_ = cb; }
private:
    TcpServer server_;
    HttpCallBack defcb_;
    std::map<std::string, std::map<std::string, HttpCallBack>> cbs_;
    void init();
};

}
