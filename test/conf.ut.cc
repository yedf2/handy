#include <handy/conf.h>
#include <string.h>
#include "test_harness.h"

using namespace std;
using namespace handy;

int dump_ini(const char* dir, const char* inifile) {
    Conf conf;
    char buf[4096];
    snprintf(buf, sizeof buf, "%s/%s", dir, inifile);
    int err = conf.parse(buf);
    if (err) {
        //printf("parse error in %s err: %d\n", inifile, err);
        return err;
    }
    //printf("file %s:\n", inifile);
    //for (auto& kv : conf.values_) {
    //    for(auto& v : kv.second) {
    //        printf("%s=%s\n", kv.first.c_str(), v.c_str());
    //    }
    //}
    return 0;
}

TEST(test::TestBase, allFiles) {
    const char* dir = "test/";
    ASSERT_EQ(1, dump_ini(dir, "files/bad_comment.ini"));
    ASSERT_EQ(1, dump_ini(dir, "files/bad_multi.ini"));
    ASSERT_EQ(3, dump_ini(dir, "files/bad_section.ini"));
    ASSERT_EQ(0, dump_ini(dir, "files/multi_line.ini"));
    ASSERT_EQ(0, dump_ini(dir, "files/normal.ini"));
    ASSERT_EQ(0, dump_ini(dir, "files/user_error.ini"));
}

