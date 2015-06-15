#include <handy/logging.h>
#include <unistd.h>
#include <time.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    for(;;) {
        error("a test error msg at %ld", time(NULL));
        sleep(1);
    }
    return 0;
}

