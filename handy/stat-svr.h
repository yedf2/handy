#pragma once

#include "slice.h"
#include "event_base.h"
#include "http.h"
#include <map>
#include <functional>

namespace handy {

typedef std::function<void(const HttpRequest&, HttpResponse&)> StatCallBack;
typedef std::function<std::string()> InfoCallBack;
typedef std::function<int64_t()> IntCallBack;

struct StatServer {
    enum StatType { STATE, PAGE, CMD, };
    StatServer(EventBase* base);
    int bind(const std::string& host, short port) { return server_.bind(host, port); }
    typedef std::string string;
    void onRequest(StatType type, const string& key, const string& desc, const StatCallBack& cb);
    void onRequest(StatType type, const string& key, const string& desc, const InfoCallBack& cb);
    //Used to display a simple state.
    void onState(const string& state, const string& desc, const InfoCallBack& cb) { onRequest(STATE, state, desc, cb); }
    void onState(const string& state, const string& desc, const IntCallBack& cb) { onRequest(STATE, state, desc, [cb] { return util::format("%ld", cb()); }); }
    //Used to display a page.
    void onPage(const string& page, const string& desc, const InfoCallBack& cb) { onRequest(PAGE, page, desc, cb); }
    void onPageFile(const string& page, const string& desc, const string& file);
    //Used to send a command.
    void onCmd(const string& cmd, const string& desc, const InfoCallBack& cb) { onRequest(CMD, cmd, desc, cb); }
    void onCmd(const string& cmd, const string& desc, const IntCallBack& cb) { onRequest(CMD, cmd, desc, [cb] { return util::format("%ld", cb()); }); }
private:
    HttpServer server_;
    typedef std::pair<std::string, StatCallBack> DescState;
    std::map<std::string, DescState> statcbs_, pagecbs_, cmdcbs_;
    std::map<std::string, StatCallBack> allcbs_;
};

}
