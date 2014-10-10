#pragma once

#include <string>
#include "status.h"

namespace handy {

struct file {
    static Status getContent(const std::string& filename, std::string& cont);
    static Status writeContent(const std::string& filename, const std::string& cont);
    static Status renameSave(const std::string& name, const std::string& tmpName, const std::string& cont);
    static Status getChildren(const std::string& dir, std::vector<std::string>* result);
    static Status deleteFile(const std::string& fname);
    static Status createDir(const std::string& name);
    static Status deleteDir(const std::string& name);
    static Status getFileSize(const std::string& fname, uint64_t* size);
    static Status renameFile(const std::string& src, const std::string& target);
    static bool fileExists(const std::string& fname);
};

}
