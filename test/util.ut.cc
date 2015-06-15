#include <handy/util.h>
#include "test_harness.h"

using namespace std;
using namespace handy;

TEST(test::TestBase, static_func) {
    ASSERT_EQ("a", util::format("a"));
    string s1 = "hello";
    for (int i = 0; i < 999; i ++) {
        s1 += "hello";
    }
    string s2 = util::format("%s", s1.c_str());
    ASSERT_EQ(1000 * 5, s2.length());
}

TEST(test::TestBase, ExitCaller) {
    ExitCaller caller1([] { printf("exit function called\n"); });
    printf("after caller\n");
}
