#pragma once
#include "string.h"
#include <string>

namespace handy {

class Slice {
public:
    Slice() : pb_("") { pe_ = pb_; }
    Slice(const char* b, const char* e):pb_(b), pe_(e) {}
    Slice(const char* d, size_t n) : pb_(d), pe_(d + n) { }
    Slice(const std::string& s) : pb_(s.data()), pe_(s.data() + s.size()) { }
    Slice(const char* s) : pb_(s), pe_(s + strlen(s)) { }

    const char* data() const { return pb_; }
    const char* begin() const { return pb_; }
    const char* end() const { return pe_; }
    char front() { return *pb_; }
    char back() { return pe_[-1]; }
    size_t size() const { return pe_ - pb_; }
    void resize(size_t sz) { pe_ = pb_ + sz; }
    inline bool empty() const { return pe_ == pb_; }
    void clear() { pe_ = pb_ = ""; }
    
    Slice eatWord();
    Slice eatLine();
    Slice eat(int sz) { Slice s(pb_, 2); pb_+=2; return s; }

    inline char operator[](size_t n) const { return pb_[n]; }

    Slice ltrim(size_t n) { Slice s(*this); s.pb_ += n; return s; }
    Slice rtrim(size_t n) { Slice s(*this); s.pe_ -= n; return s; }
    Slice trimSpace();

    std::string toString() const { return std::string(pb_, pe_); }

    // Three-way comparison.  Returns value:
    int compare(const Slice& b) const;

    // Return true if "x" is a prefix of "*this"
    bool starts_with(const Slice& x) const {
        return (size() > x.size() && memcmp(pb_, x.pb_, x.size()) == 0); 
    }

    bool end_with(const Slice& x) const {
        return (size() > x.size() && memcmp(pe_ - x.size(), x.pb_, x.size()) == 0); 
    }
    operator std::string() { return std::string(pb_, pe_); }
private:
    const char* pb_;
    const char* pe_;
};

inline Slice Slice::eatWord() {
    const char* b = pb_;
    while (b < pe_ && isspace(*b)) {
        b++;
    }
    const char* e = b;
    while (e < pe_ && !isspace(*e)) {
        e ++;
    }
    pb_ = e;
    return Slice(b, e-b);
}

inline Slice Slice::eatLine() { 
    const char* p = pb_; 
    while (pb_<pe_ && *pb_ != '\n' && *pb_!='\r') {
        pb_++; 
    }
    return Slice(p, pb_-p); 
}

inline Slice Slice::trimSpace() {
    Slice r(*this);
    while (r.pb_ < r.pe_ && isspace(*r.pb_)) r.pb_ ++;
    while (r.pb_ < r.pe_ && isspace(r.pe_[-1])) r.pe_ --;
    return r;
}

inline bool operator < (const Slice& x, const Slice& y) {
    return x.compare(y) < 0;
}

inline bool operator==(const Slice& x, const Slice& y) {
    return ((x.size() == y.size()) &&
        (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const Slice& x, const Slice& y) {
    return !(x == y);
}

inline int Slice::compare(const Slice& b) const {
    size_t sz = size(), bsz = b.size();
    const int min_len = (sz < bsz) ? sz : bsz;
    int r = memcmp(pb_, b.pb_, min_len);
    if (r == 0) {
        if (sz < bsz) r = -1;
        else if (sz > bsz) r = +1;
    }
    return r;
}

}
