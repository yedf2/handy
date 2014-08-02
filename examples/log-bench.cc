#include "handy.h"
#include "conf.h"
#include "logging.h"
#include "daemon.h"

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    Logger::getLogger().setFileName("/dev/null");
    //Logger::getLogger().setRotateInterval(60);
    time_t last = time(NULL);
    long cn = 0, lastcn = 0;
    for(;;) {
        for (int i = 0; i< 1000; i++) {
            info("msgid %ld", ++cn);
            //info("msg"); cn++;
        }
        time_t cur = time(NULL);
        if (cur != last) {
            last = cur;
            printf("log qps %ld\n", (cn - lastcn));
            lastcn = cn;
        }
    }
}