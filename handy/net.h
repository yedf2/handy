#pragma once
#include <string>
namespace handy {

struct Ip4Addr {
    Ip4Addr(const char* host, short port);
    Ip4Addr(short port=0): Ip4Addr("", port) {}
    Ip4Addr(const struct sockaddr_in& addr): addr_(addr) {};
    std::string toString() const;
    std::string ip() const;
    short port() const;
    unsigned int ipInt() const;
    //if you pass a hostname to constructor, then use this to check error
    bool isIpValid() const;
    struct sockaddr_in& getAddr() { return addr_; }
private:
    struct sockaddr_in addr_;
};

struct Buffer {
    Buffer(): buf_(NULL), b_(0), e_(0), cap_(0), exp_(4096) {}
    ~Buffer() { delete[] buf_; }
    void clear() { if (cap_ > SmallBuf) { delete[] buf_; buf_ = NULL; cap_ = 0; } b_ = e_ = 0; }
    size_t size() const { return e_ - b_; }
    bool empty() const  { return e_ == b_; }
    char* data() const  { return buf_ + b_; }
    char* begin() const  { return buf_ + b_; }
    char* end() const  { return buf_ + e_; }
    char* makeRoom(size_t len);
    void makeRoom() { if (space() < exp_) expand(0);}
    size_t space() const  { return cap_ - e_; }
    void addSize(size_t len) { e_ += len; }
    char* allocRoom(size_t len) { char* p = makeRoom(len); addSize(len); return p; }
    Buffer& append(const char* p, size_t len) { memcpy(allocRoom(len), p, len); return *this; }
    template<class T> Buffer& appendValue(const T& v) { append((const char*)&v, sizeof v); return *this; }
    Buffer& consume(size_t len) { b_ += len; if (size() == 0) clear(); return *this; }
    Buffer& absorb(Buffer& buf);
    void setSuggestSize(size_t sz) { exp_ = sz; }
    Buffer(const Buffer& b) { copyFrom(b); }
    Buffer& operator=(const Buffer& b) { delete[] buf_; buf_ = NULL; copyFrom(b); return *this; }
private:
    enum { SmallBuf = 1024, };
    char* buf_;
    size_t b_, e_, cap_, exp_;
    void moveHead() { std::copy(begin(), end(), buf_); e_ -= b_; b_ = 0; }
    void expand(size_t len);
    void copyFrom(const Buffer& b);
};

inline char* Buffer::makeRoom(size_t len) {
    if (e_ + len <= cap_) {
    } else if (size() + len < cap_ / 2) {
        moveHead();
    } else {
        expand(len);
    }
    return end();
}

inline void Buffer::expand(size_t len) {
    size_t ncap = std::max(exp_, std::max(2*cap_, size()+len));
    char* p = new char[ncap];
    std::copy(begin(), end(), p);
    e_ -= b_;
    b_ = 0;
    delete[] buf_;
    buf_ = p;
    cap_ = ncap;
}

inline void Buffer::copyFrom(const Buffer& b) {
    memcpy(this, &b, sizeof b); 
    if (size()) { 
        buf_ = new char[cap_]; 
        memcpy(data(), b.begin(), b.size());
    }
}

inline Buffer& Buffer::absorb(Buffer& buf) { 
    if (&buf != this) {
        if (size() == 0) {
            char b[sizeof buf];
            memcpy(b, this, sizeof b);
            memcpy(this, &buf, sizeof b);
            memcpy(&buf, b, sizeof b);
            std::swap(exp_, buf.exp_); //keep the origin exp_
        } else {
            append(buf.begin(), buf.size());
            buf.clear();
        }
    }
    return *this;
}

}