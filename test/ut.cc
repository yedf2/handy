#include <handy/logging.h>
#include "test_harness.h"

int main(int argc, char** argv) {
    char junk = 0;
    for (int i = 1; i < argc; i ++) {
        if (sscanf(argv[i], "-%c", &junk) == 1) {
            if (junk == 'h') {
                printf("%s [options] [matcher]\n", argv[0]);
                printf("options:\n\t-v verbose mode\n\t-h help\n");
                printf("matcher:\n\tonly run test contain 'matcher'\n");
                return 0;
            } else if (junk == 'v') {
                handy::Logger::getLogger().setLogLevel("TRACE");
            } else {
                printf("unknown option");
                return 1;
            }
        } else {
            handy::test::RunAllTests(argv[i]);
            return 0;
        }
    }
    handy::test::RunAllTests(NULL);
    return 0;
}