#pragma once

#include "slice.h"
#include "handy.h"
#include <map>

namespace handy {

struct HttpResponse {
    void encodeTo(Buffer& buf) const;
    std::map<std::string, std::string> headers;
    std::string body;
    int status;
    std::string statusWord;
    HttpResponse():status(200), statusWord("OK") {}
    void setNotFound() { status = 404; body = "Not Found"; statusWord="Not Found"; }
};

struct HttpRequest {
    //return false on error
    bool update(Buffer& buf);
    bool isComplete() const { return complete_; }
    void eat(Buffer& buf) { buf.consume(scanned_); }
    void clear();
    HttpRequest() { clear(); }
    std::string getQuery(const std::string& q);
    std::map<std::string, std::string> headers, query;
    std::string method, uri, query_uri, version;
    Slice body;
private:
    size_t scanned_;
    bool complete_;
    size_t contentLen_;
};

struct HttpConn {
    HttpConn(const TcpConnPtr& con): con_(con){}
    void close() const { con_->close(); }
    void send(const HttpResponse& resp) const { Buffer& b = con_->getOutput(); resp.encodeTo(b); con_->send(b); }
    HttpRequest& getRequest() const { return con_->context<HttpRequest>(); }
private:
    const TcpConnPtr& con_;
};

typedef std::function<void(const HttpConn&)> HttpCallBack;
struct HttpServer {
    HttpServer(EventBase* base, Ip4Addr addr);
    void onGet(const std::string& uri, HttpCallBack cb) { gets_[uri] = cb; }
    void onRequest(const std::string& method, const std::string& uri, HttpCallBack cb);
    void onDefault(HttpCallBack cb) { defcb_ = cb; }
private:
    TcpServer server_;
    HttpCallBack defcb_;
    std::map<std::string, HttpCallBack> gets_;
    std::map<std::string, std::map<std::string, HttpCallBack>> cbs_;
};

}