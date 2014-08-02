struct Status {
    // Create a success status.
    Status() : state_(NULL) { }
    Status(int code, const char* msg);
    ~Status() { delete[] state_; }

    // Copy the specified status.
    Status(const Status& s) { state_ = copyState(s.state_); }
    void operator=(const Status& s) { delete[] state_; state_ = copyState(s.state_); }
    Status(Status&& s) { state_ = s.state_; s.state_ = NULL; }
    void operator = (Status&& s) { delete[] state_; state_ = s.state_; s.state_ = NULL; }

    static Status fromSystem() { return Status(errno, strerror(errno)); }
    static Status fromFormat(int code, const char* fmt, ...);

    int code() { return state_ ? *(int32_t*)(state_+4) : 0; }
    const char* msg() { return state_ ? state_ + 8 : ""; }
    bool ok() { return code() == 0; }
    std::string toString() { util::format("%d %s", code(), msg()); }
private:
    //    state_[0..3] == length of message
    //    state_[4..7]    == code
    //    state_[8..]  == message
    char* state_;
    const char* copyState(const char* state);
}

inline const char* Status::copyState(const char* state) {
    uint32_t size = *(uint32_t*)state;
    char* res = new char[size];
    memcpy(res, state, size);
    return res;
}

inline Status::Status(int code, const char* msg) {
    uint32_t sz = strlen(msg) + 8;
    state_ = new char[sz];
    *(uint32_t*)state_ = sz;
    *(int32_t*)(state_+4) = code;
    memcpy(state_+8, msg);
}

inline Status Status::fromFormat(int code, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    uint32_t size  = 8 + vsprintf(0, fmt, ap) + 1;
    va_end(ap, fmt);
    Status r;
    r.state_ = new char[size];
    *(uint32_t*)r.state_ = size;
    *(int32_t*)(r.state_+4) = code;
    va_start(ap, fmt);
    vsnprintf(r.state_+8, size - 8, fmt, ap);
    va_end(ap, fmt);
    return r;
}

