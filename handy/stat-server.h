#pragma once

#include "slice.h"
#include "handy.h"
#include "http.h"
#include <map>

namespace handy {

typedef std::function<void(const HttpRequest&, HttpResponse&)> StatCallBack;
typedef std::function<std::string()> InfoCallBack;

struct StatServer {
    enum StatType { STATE, PAGE, CMD, };
    StatServer(EventBase* base, Ip4Addr addr);
    StatServer(EventBase* base, const std::string& host, int port):StatServer(base, Ip4Addr(host, port)) {};
    typedef std::string string;
    void onRequest(StatType type, const string& key, const string& desc, const StatCallBack& cb);
    void onRequest(StatType type, const string& key, const string& desc, const InfoCallBack& cb);
    void onState(const string& state, const string& desc, const InfoCallBack& cb) { onRequest(STATE, state, desc, cb); }
    void onPage(const string& page, const string& desc, const InfoCallBack& cb) { onRequest(PAGE, page, desc, cb); }
    void onCmd(const string& cmd, const string& desc, const InfoCallBack& cb) { onRequest(CMD, cmd, desc, cb); }
private:
    HttpServer server_;
    typedef std::pair<std::string, StatCallBack> DescState;
    std::map<std::string, DescState> statcbs_, pagecbs_, cmdcbs_;
    std::map<std::string, StatCallBack> allcbs_;
};

}
