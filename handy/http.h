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
    void clear();

    std::string getArg(const std::string& n) { return map_get(args, n); }
    std::map<std::string, std::string> args;
    std::string method, uri, query_uri;
};

struct HttpResponse: public HttpMsg {
    HttpResponse() { clear(); }
    //return how many byte has been encoded
    int encode(Buffer& buf);
    Result tryDecode(Slice buf, bool copyBody=true);
    void clear() { status = 200; statusWord = "OK"; }

    void setNotFound() { status = 404; body2 = Slice("Not Found"); statusWord="Not Found"; }
    std::string statusWord;
    int status;
};




struct HttpConn {
    HttpConn(const TcpConnPtr& con): con_(con){}
    void close() { con_->close(); }
    void send(HttpResponse& resp) const { Buffer& b = con_->getOutput(); resp.encode(b); con_->send(b); }
    HttpRequest& getRequest() const { return con_->context<HttpRequest>(); }
private:
    TcpConnPtr con_;
};

typedef std::function<void(const HttpConn&)> HttpCallBack;

struct HttpServer {
    HttpServer(EventBase* base, Ip4Addr addr);
    void onGet(const std::string& uri, HttpCallBack cb) { cbs_["GET"][uri] = cb; }
    void onRequest(const std::string& method, const std::string& uri, HttpCallBack cb) { cbs_[method][uri] = cb; }
    void onDefault(HttpCallBack cb) { defcb_ = cb; }
private:
    TcpServer server_;
    void handleRead(const TcpConnPtr& con);
    HttpCallBack defcb_;
    std::map<std::string, std::map<std::string, HttpCallBack>> cbs_;
};

}
