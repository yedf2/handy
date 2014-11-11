#include "ssl-conn.h"
#include <assert.h>
#include <logging.h>
#include <poll.h>

//#define SSL_ERROR_NONE 0
//#define SSL_ERROR_SSL 1
//#define SSL_ERROR_WANT_READ 2
//#define SSL_ERROR_WANT_WRITE 3
//#define SSL_ERROR_WANT_X509_LOOKUP 4
//#define SSL_ERROR_SYSCALL 5
//#define SSL_ERROR_ZERO_RETURN 6
//#define SSL_ERROR_WANT_CONNECT 7
//#define SSL_ERROR_WANT_ACCEPT 8

using namespace std;

namespace handy {

namespace {
    static BIO* errBio;
    static SSL_CTX* g_sslCtx;

    void safeSSLInit() {
        static int forinit = [] {
            // Register the error strings for libcrypto & libssl
            SSL_load_error_strings ();
            // Register the available ciphers and digests
            int r = SSL_library_init ();
            fatalif(!r, "SSL_library_init failed");
            g_sslCtx = SSL_CTX_new (SSLv23_method ());
            fatalif(g_sslCtx == NULL, "SSL_CTX_new failed");
            errBio = BIO_new_fd(Logger::getLogger().getFd(), BIO_NOCLOSE);
            static ExitCaller freeBio([]{ 
                BIO_free(errBio);
                SSL_CTX_free(g_sslCtx);

            });
            trace("ssl library inited");
            return 0;
        }();
        (void)forinit;
    }
}

SSLConn::SSLConn():
TcpConn(), ssl_(NULL), tcpConnected_(false) {
    safeSSLInit();
}

SSLConn::~SSLConn() {
    if (ssl_) {
        SSL_shutdown (ssl_);
        SSL_free(ssl_);
    }
}

int SSLConn::handleHandshake(const TcpConnPtr& con) {
    fatalif(state_ != State::Handshaking, "unexpect state %d", state_);
    if (!tcpConnected_) {
        struct pollfd pfd;
        pfd.fd = channel_->fd();
        pfd.events = POLLOUT | POLLERR;
        int r = poll(&pfd, 1, 0);
        if (r == 1 && pfd.revents == POLLOUT) {
            channel_->enableReadWrite(true, true);
            trace("tcp connected %s - %s fd %d",
                local_.toString().c_str(), peer_.toString().c_str(), channel_->fd());
            tcpConnected_ = true;
        } else {
            trace("poll fd %d return %d revents %d", channel_->fd(), r, pfd.revents);
            cleanup(con);
            return -1;
        }
    }
    if (ssl_ == NULL) {
        ssl_ = SSL_new (g_sslCtx);
        fatalif(ssl_ == NULL, "SSL_new failed");
        int r = SSL_set_fd(ssl_, channel_->fd());
        fatalif(!r, "SSL_set_fd failed");
        if (isClient_) {
            trace("SSL_set_connect_state for channel %ld fd %d", channel_->id(), channel_->fd());
            SSL_set_connect_state(ssl_);
        } else {
            trace("SSL_set_accept_state for channel %ld fd %d", channel_->id(), channel_->fd());
            SSL_set_accept_state(ssl_);
        }
    }
    int r = SSL_do_handshake(ssl_);
    if (r == 1) {
        state_ = State::Connected;
        trace("ssl connected %s - %s fd %d",
            local_.toString().c_str(), peer_.toString().c_str(), channel_->fd());
        if (statecb_) {
            statecb_(con);
        }
        return 0;
    }
    int err = SSL_get_error(ssl_, r);
    if (err == SSL_ERROR_WANT_WRITE) {
        if (!channel_->writeEnabled())
            channel_->enableWrite(true);
    } else if (err == SSL_ERROR_WANT_READ) {
        if (channel_->writeEnabled())
            channel_->enableWrite(false);
    } else {
        error("SSL_do_handshake return %d error %d errno %d msg %s", r, err, errno, strerror(errno));
        ERR_print_errors(errBio);
        cleanup(con);
        return -1;
    }
    return 0;
}

int SSLConn::readImp(int fd, void* buf, size_t bytes) {
    int rd = SSL_read(ssl_, buf, bytes);
    int ssle = SSL_get_error(ssl_, rd);
    if (rd < 0 && ssle != SSL_ERROR_WANT_READ) {
        error("SSL_read return %d error %d errno %d msg %s", rd, ssle, errno, strerror(errno));
    }
    return rd;
}

int SSLConn::writeImp(int fd, const void* buf, size_t bytes) {
    int wd = SSL_write(ssl_, buf, bytes);
    int ssle = SSL_get_error(ssl_, wd);
    if (wd < 0 && ssle != SSL_ERROR_WANT_WRITE) {
        error("SSL_write return %d error %d errno %d msg %s", wd, ssle, errno, strerror(errno));
    }
    return wd;
}

bool SSLConn::certKeyInited_ = false;

void SSLConn::setSSLCertKey(const std::string& cert, const std::string& key) {
    safeSSLInit();
    int r = SSL_CTX_use_certificate_file(g_sslCtx, cert.c_str(), SSL_FILETYPE_PEM);
    fatalif(r<=0, "SSL_CTX_use_certificate_file %s failed", cert.c_str());
    r = SSL_CTX_use_PrivateKey_file(g_sslCtx, key.c_str(), SSL_FILETYPE_PEM);
    fatalif(r<=0, "SSL_CTX_use_PrivateKey_file %s failed", key.c_str());
    r = SSL_CTX_check_private_key(g_sslCtx);
    fatalif(!r, "SSL_CTX_check_private_key failed");
    trace("ssl cert %s key %s", cert.c_str(), key.c_str());
    certKeyInited_ = true;
}

};