#include "conn.h"
#include "logging.h"
#include "daemon.h"
#include "codec.h"
#include <map>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    for(;;) {
        error("a test error msg at %ld", time(NULL));
        sleep(1);
    }
    return 0;
}

