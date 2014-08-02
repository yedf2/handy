#pragma once
#include <string>

namespace handy {

class Slice {
public:
    Slice() : pb_("") { pe_ = pb_; }
    Slice(const char* d, size_t n) : pb_(d), pe_(d + n) { }
    Slice(const std::string& s) : pb_(s.data()), pe_(s.data() + s.size()) { }
    Slice(const char* s) : pb_(s), pe_(s + strlen(s)) { }

    const char* data() const { return pb_; }
    const char* end() const { return pe_; }
    size_t size() const { return pe_ - pb_; }
    void resize(size_t sz) { pe_ = pb_ + sz; }
    inline bool empty() const { return pe_ == pb_; }
    void clear() { pe_ = pb_ = ""; }

    inline char operator[](size_t n) const { return pb_[n]; }

    void consume(size_t n) { pb_ += n; }

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

    Slice& append(const char* d, size_t sz) { memcpy(pe_, d, sz); pe_ += sz; return *this; }

private:
    const char* pb_;
    const char* pe_;
};

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