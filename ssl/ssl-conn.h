#pragma once
#include <conn.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace handy {

struct SSLConn : public TcpConn {
    SSLConn();
    ~SSLConn();
    static void setSSLCertKey(const std::string& cert, const std::string& key);
protected:
    SSL *ssl_;
    bool tcpConnected_;
    virtual int readImp(int fd, void* buf, size_t bytes);
    virtual int writeImp(int fd, const void* buf, size_t bytes);
    virtual int handleHandshake(const TcpConnPtr& con);
    static bool certKeyInited_;
};

};
