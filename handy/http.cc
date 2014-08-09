#include "http.h"
#include "logging.h"

using namespace std;

namespace handy {

bool HttpRequest::update(Buffer& buf) {
    if (complete_) {
        return true;
    }
    if (!contentLen_) {
        char* p = buf.begin();
        Slice req;
        //scanned at most points to first <cr> when empty line
        while (buf.size() >= scanned_ + 4) {
            if (p[scanned_] == '\r' && memcmp(p+scanned_, "\r\n\r\n", 4) == 0) {
                req = Slice(p, p+scanned_);
                break;
            }
            scanned_++;
        }
        if (req.empty()) { // header not complete
            return true;
        }

        Slice ln1 = req.eatLine();
        method = ln1.eatWord();
        query_uri = ln1.eatWord();
        version = ln1.eatWord();
        if (query_uri.size() == 0 || query_uri[0] != '/') {
            error("query uri '%.*s' should begin with /", (int)query_uri.size(), query_uri.data());
            return false;
        }
        for (size_t i = 0; i <= query_uri.size(); i++) {
            if (query_uri[i] == '?') {
                uri = Slice(query_uri.data(), i);
                Slice qs = Slice(query_uri.data()+i+1, query_uri.size()-i-1);
                size_t c, kb, ke, vb, ve;
                ve = vb = ke = kb = c = 0;
                while (c < qs.size()) {
                    while (c < qs.size() && qs[c] != '=' && qs[c] != '&') c++;
                    ke = c;
                    if (c < qs.size() && qs[c] == '=') c++;
                    vb = c;
                    while (c < qs.size() && qs[c] != '&') c++;
                    ve = c;
                    if (c < qs.size() && qs[c] == '&') c++;
                    if (kb != ke) {
                        query[string(qs.data()+kb, qs.data()+ke)] =
                            string(qs.data()+vb, qs.data()+ve);
                    }
                    ve = vb = ke = kb = c;
                }
                break;
            }
            if (i == query_uri.size()) {
                uri = query_uri;
            }
        }
        while (req.size()) {
            Slice ln = req.eatLine();
            Slice k = req.eatWord();
            Slice w = req.eatWord();
            if (k.size() && w.size() && k.back() == ':') {
                headers[k.rtrim(1)] = w;
            } else if (k.empty() && w.empty() && req.empty()) {
                break;
            } else {
                error("bad http line: %.*s", (int)ln.size(), ln.data());
                return false;
            }
        }
        req.eatLine();
        scanned_ += 4;
        auto it = headers.find("Content-Length");
        if (it == headers.end()) {
            complete_ = true;
        } else {
            contentLen_ = atoi(it->second.data());
        }
    }
    if (!complete_ && buf.size() >= contentLen_ + scanned_) {
        body = Slice(buf.data() + scanned_, contentLen_);
        scanned_ += contentLen_;
        complete_ = true;
    }
    return true;
}

string HttpRequest::getQuery(const string& q) {
    auto p = query.find(q);
    return p == query.end() ? "" : p->second;
}

void HttpRequest::clear() {
    scanned_ = 0;
    complete_ = false;
    body = Slice();
    contentLen_ = 0;
    query.clear();
    headers.clear();
}

void HttpResponse::encodeTo(Buffer& buf) const {
    char conlen[1024], statusln[1024];
    snprintf(statusln, sizeof statusln, 
        "HTTP/1.1 %d %s\r\n", status, statusWord.c_str());
    buf.append(statusln);
    for (auto& hd: headers) {
        buf.append(hd.first).append(": ").append(hd.second).append("\r\n");
    }
    buf.append("Connection: Keep-Alive\r\n");
    snprintf(conlen, sizeof conlen, "Content-Length: %lu\r\n", body.size());
    buf.append(conlen);
    buf.append("\r\n").append(body);
}

HttpServer::HttpServer(EventBase* base, Ip4Addr addr): server_(base, addr) {
    server_.onConnState([](const TcpConnPtr& con) {
        if (con->getState() == TcpConn::Connected) {
            con->setContext(new HttpRequest());
        } else if (con->getState() == TcpConn::Closed) {
            delete (HttpRequest*)con->getContext();
        }
    });
    defcb_ = [](const HttpConn& con) {
        HttpResponse resp;
        resp.status = 404;
        resp.statusWord = "Not Found";
        resp.body = "Not Found";
        con.send(resp);
    };
    server_.onConnRead([this](const TcpConnPtr& con){
        HttpRequest* req = (HttpRequest*)con->getContext();
        Buffer& input = con->getInput();
        bool r = req->update(input);
        if (!r) {
            con->close();
            return;
        }
        if (req->isComplete()) {
            ExitCaller call1([&]{req->eat(input); req->clear();});
            info("http request: %s %s %s", req->method.c_str(), 
                req->query_uri.c_str(), req->version.c_str());
            HttpConn hcon(con);
            if (req->method == "GET") {
                auto p = gets_.find(req->uri);
                if (p != gets_.end()) {
                    p->second(hcon);
                    return;
                }
            } else {
                auto p = cbs_.find(req->method);
                if (p != cbs_.end()) {
                    auto p2 = p->second.find(req->uri);
                    if (p2 != p->second.end()) {
                        p2->second(hcon);
                        return;
                    }
                }
            }
            defcb_(hcon);
        }
    });
}


}