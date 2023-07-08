

#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>

class Epoll
{
public:
    static int CreateEpoll()
    {
        int epfd = epoll_create(__gsize);
        if(epfd > 0) return epfd;
        exit(5); // epoll模型创建失败，直接终止
    }
private:
    static const int __gsize = 256;
public:
    static bool CtlEpoll(int epfd, int oper, int sock, uint32_t events)
    {
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = sock;
        int n = epoll_ctl(epfd, oper, sock, &ev);
        return n == 0;
    }
    static int WaitEpoll(int epfd, struct epoll_event revs[], int num, int timeout)
    {
        return epoll_wait(epfd, revs, num, timeout);
    }
};