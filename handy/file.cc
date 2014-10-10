#include "file.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

namespace handy {

Status file::getContent(const std::string& filename, std::string& cont){
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        return Status::ioError("open", filename);
    }
    ExitCaller ec1([=]{ close(fd); });
    char buf[4096];
    for(;;) {
        int r = read(fd, buf, sizeof buf);
        if (r < 0) {
            return Status::ioError("read", filename);
        } else if (r == 0) {
            break;
        }
        cont.append(buf, r);
    }
    return Status();
}

Status file::writeContent(const std::string& filename, const std::string& cont) {
    int fd = open(filename.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return Status::ioError("open", filename);
    }
    ExitCaller ec1([=]{ close(fd); });
    int r = write(fd, cont.data(), cont.size());
    if (r < 0) {
        return Status::ioError("write", filename);
    }
    return Status();
}

Status file::renameSave(const string& name, const string& tmpName, const string& cont) {
    Status s = writeContent(tmpName, cont);
    if (s.ok()) {
        unlink(name.c_str());
        s = renameFile(tmpName, name);
    }
    return s;
}

Status file::getChildren(const std::string& dir, std::vector<std::string>* result) {
    result->clear();
    DIR* d = opendir(dir.c_str());
    if (d == NULL) {
        return Status::ioError("opendir", dir);
    }
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        result->push_back(entry->d_name);
    }
    closedir(d);
    return Status();
}

Status file::deleteFile(const string& fname) {
    if (unlink(fname.c_str()) != 0) {
        return Status::ioError("unlink", fname);
    }
    return Status();
}

Status file::createDir(const std::string& name) {
    if (mkdir(name.c_str(), 0755) != 0) {
        return Status::ioError("mkdir", name);
    }
    return Status();
}

Status file::deleteDir(const std::string& name) {
    if (rmdir(name.c_str()) != 0) {
        return Status::ioError("rmdir", name);
    }
    return Status();
}

Status file::getFileSize(const std::string& fname, uint64_t* size) {
    struct stat sbuf;
    if (stat(fname.c_str(), &sbuf) != 0) {
        *size = 0;
        return Status::ioError("stat", fname);
    } else {
        *size = sbuf.st_size;
    }
    return Status();
}

Status file::renameFile(const std::string& src, const std::string& target) {
    if (rename(src.c_str(), target.c_str()) != 0) {
        return Status::ioError("rename", src + " " + target);
    }
    return Status();
}

bool file::fileExists(const std::string& fname) {
    return access(fname.c_str(), F_OK) == 0;
}

}
