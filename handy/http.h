#pragma once

#include "slice.h"
#include "conn.h"
#include <map>

namespace handy {

//base class for HttpRequest and HttpResponse
struct HttpMsg {
    enum Result{ Error, Complete, NotComplete, Continue100, };
    HttpMsg() { HttpMsg::clear(); };

    //Add content to `buf`, return the number of bytes written.
    virtual int encode(Buffer& buf)=0;
    //Try to parse from `buf`, copy body content by default.
    virtual Result tryDecode(Slice buf, bool copyBody=true)=0;
    //Clear the message related fields.
    virtual void clear();

    std::map<std::string, std::string> headers;
    std::string version, body;
    //The `body` may be large, in order to avoid data copying, add `body2`.
    Slice body2;

    std::string getHeader(const std::string& n) { return map_get(headers, n); }
    Slice getBody() { return body2.size() ? body2 : (Slice)body; }

    //Return the number of bytes prased if tryDecode() return `Complete`.
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

//The Http connection is essentially a tcp connection. 
//The following package is mainly added to the HttpRequest, HttpResponse processing.
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
    //Response a file.
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

//Http server.
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
