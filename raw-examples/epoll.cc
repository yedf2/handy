#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define exit_if(r, ...) if(r) {printf(__VA_ARGS__); printf("error no: %d error msg %s\n", errno, strerror(errno)); exit(1);}

void setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    exit_if(flags<0, "fcntl failed");
    int r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    exit_if(r<0, "fcntl failed");
}

void updateEvents(int efd, int fd, int events, int op) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    printf("%s fd %d events read %d write %d\n",
           op==EPOLL_CTL_MOD?"mod":"add", fd, ev.events & EPOLLIN, ev.events & EPOLLOUT);
    int r = epoll_ctl(efd, op, fd, &ev);
    exit_if(r, "epoll_ctl failed");
}

void handleAccept(int efd, int fd) {
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int cfd = accept(fd,(struct sockaddr *)&raddr,&rsz);
    exit_if(cfd<0, "accept failed");
    sockaddr_in peer, local;
    socklen_t alen = sizeof(peer);
    int r = getpeername(cfd, (sockaddr*)&peer, &alen);
    exit_if(r<0, "getpeername failed");
    printf("accept a connection from %s\n", inet_ntoa(raddr.sin_addr));
    setNonBlock(cfd);
    updateEvents(efd, cfd, EPOLLIN|EPOLLOUT, EPOLL_CTL_ADD);
}

void handleRead(int efd, int fd) {
    char buf[4096];
    int n = 0;
    while ((n=::read(fd, buf, sizeof buf)) > 0) {
        printf("read %d bytes\n", n);
        int r = ::write(fd, buf, n); //写出读取的数据
        //实际应用中，写出数据可能会返回EAGAIN，此时应当监听可写事件，当可写时再把数据写出
        exit_if(r<=0, "write error");
    }
    if (n<0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;
    exit_if(n<0, "read error"); //实际应用中，n<0应当检查各类错误，如EINTR
    printf("fd %d closed\n", fd);
    close(fd);
}

void handleWrite(int efd, int fd) {
    //实际应用应当实现可写时写出数据，无数据可写才关闭可写事件
    updateEvents(efd, fd, EPOLLIN, EPOLL_CTL_MOD);
}

void loop_once(int efd, int lfd, int waitms) {
    const int kMaxEvents = 20;
    struct epoll_event activeEvs[100];
    int n = epoll_wait(efd, activeEvs, kMaxEvents, waitms);
    printf("epoll_wait return %d\n", n);
    for (int i = 0; i < n; i ++) {
        int fd = activeEvs[i].data.fd;
        int events = activeEvs[i].events;
        if (events & (EPOLLIN | EPOLLERR)) {
            if (fd == lfd) {
                handleAccept(efd, fd);
            } else {
                handleRead(efd, fd);
            }
        } else if (events & EPOLLOUT) {
            handleWrite(efd, fd);
        } else {
            exit_if(1, "unknown event");
        }
    }
}

int main() {
    short port = 99;
    int epollfd = epoll_create(1);
    exit_if(epollfd < 0, "epoll_create failed");
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    exit_if(listenfd < 0, "socket failed");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int r = ::bind(listenfd,(struct sockaddr *)&addr, sizeof(struct sockaddr));
    exit_if(r, "bind to 0.0.0.0:%d failed %d %s", port, errno, strerror(errno));
    r = listen(listenfd, 20);
    exit_if(r, "listen failed %d %s", errno, strerror(errno));
    printf("fd %d listening at %d\n", listenfd, port);
    setNonBlock(listenfd);
    updateEvents(epollfd, listenfd, EPOLLIN, EPOLL_CTL_ADD);
    for (;;) { //实际应用应当注册信号处理函数，退出时清理资源
        loop_once(epollfd, listenfd, 10000);
    }
    return 0;
}
