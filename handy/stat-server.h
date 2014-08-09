#pragma once

#include "slice.h"
#include "handy.h"
#include <map>

namespace handy {

typedef std::function<void(const HttpRequest&, HttpResponse&)> StatCallBack;

struct StatServer {
    StatServer(EventBase* base, Ip4Addr addr);
    void onState(const std::string& query, const std::string& desc, StatCallBack cb) { statcbs_[query] = { desc, cb }; allcbs_[query] = cb; }
    void onPage(const std::string& page, const std::string& desc, StatCallBack cb) { pagecbs_[page] = {desc, cb}; allcbs_[page] = cb; }
    void onCmd(const std::string& cmd, const std::string& desc, StatCallBack cb) { cmdcbs_[cmd] = {desc, cb}; allcbs_[cmd] = cb; }
private:
    HttpServer server_;
    typedef std::pair<std::string, StatCallBack> DescState;
    std::map<std::string, DescState> statcbs_, pagecbs_, cmdcbs_;
    std::map<std::string, StatCallBack> allcbs_;
};

}
