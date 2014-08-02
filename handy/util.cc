#include "util.h"
#include <stdarg.h>
#include <memory>
#include <algorithm>
#include <chrono>

using namespace std;

namespace handy {

string util::format(const char* fmt, ...) {
    char buffer[500];
    unique_ptr<char[]> release1;
    char* base;
    for (int iter = 0; iter < 2; iter++) {
        int bufsize;
        if (iter == 0) {
            bufsize = sizeof(buffer);
            base = buffer;
        } else {
            bufsize = 30000;
            base = new char[bufsize];
            release1.reset(base);
        }
        char* p = base;
        char* limit = base + bufsize;
        if (p < limit) {
            va_list ap;
            va_start(ap, fmt);
            p += vsnprintf(p, limit - p, fmt, ap);
            va_end(ap);
        }
        // Truncate to available space if necessary
        if (p >= limit) {
            if (iter == 0) {
                continue;       // Try again with larger buffer
            } else {
                p = limit - 1;
                *p = '\0';
            }
        }
        break;
    }
    return base;
}

int64_t util::timeMicro() {
    chrono::time_point<chrono::system_clock> p = chrono::system_clock::now();
    return chrono::duration_cast<chrono::microseconds>(p.time_since_epoch()).count();
}
int64_t util::steadyMicro() {
    chrono::time_point<chrono::steady_clock> p = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::microseconds>(p.time_since_epoch()).count();
}
}