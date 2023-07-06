
#ifndef __POLL_SVR_H__
#define __POLL_SVR_H__

#include <iostream>
#include <poll.h>
#include "Sock.hpp"
#include "Log.hpp"
#include <sys/time.h>
#include <string>
#include <vector>

#define FD_NONE -1 // 如果文件描输入不知道的时候，统一设置成-1

// select的服务器 我们只完成读取 写入和异常先不做处理，这些我们到epoll里面再写完整
class PollServer
{
public:
    static const int ndfs = 100;
public:
    PollServer(const uint16_t &port = 8080) : __port(port), __nfds(ndfs)
    {
        __listen_sock = Sock::Socket();
        Sock::Bind(__listen_sock, __port);
        Sock::Listen(__listen_sock);
        logMessage(DEBUG, "create base socket success");

        __fds = new struct pollfd[__nfds]; // 把这个结构体数组搞一下
        // 把结构体里面的东西初始化一下
        for(int i = 0; i < __nfds; i++)
        {
            __fds[i].fd = FD_NONE;
            __fds[i].events = __fds[i].revents = 0;
        }
        // 可以先把listensock放进来先
        __fds[0].fd = __listen_sock;
        __fds[0].events = POLLIN;
        // 把timeout设置一下
        __timeout = 1000;
    }
    ~PollServer()
    {
        if (__listen_sock >= 0)
            close(__listen_sock);
        if (__fds) delete[] __fds;
    }
    void Start()
    {
        while (true)
        {
            int n = poll(__fds, __nfds, __timeout);
            switch (n)
            {
            case 0: // 超时
                logMessage(DEBUG, "time out...");
                break;
            case -1: // 等待失败
                logMessage(WARNING, "select error: %d : %s", errno, strerror(errno));
                break;
            default:
                // 成功的
                logMessage(DEBUG, "get a new link event ..."); 
                HandlerEvent();
                break;
            }
        }
    }
private:
    void Accepter()
    {
        std::string client_ip;
        uint16_t client_port;
        int sock = Sock::Accept(__listen_sock, &client_ip, &client_port);
        if (sock < 0)
        {
            logMessage(WARNING, "accept error");
            return;
        }
        logMessage(DEBUG, "get a new link success : [%s:%d] : %d", client_ip.c_str(), client_port, sock);

        int pos = 1;
        for (pos = 1; pos < __nfds; pos++)
        {
            if (__fds[pos].fd == FD_NONE)
            {
                // 当前位置没有被占用
                break;
            }
        }
        if (pos == __nfds)
        {
            // 满了
            logMessage(WARNING, "%s:%d", "poll server already full, closs: %d", sock);
            close(sock);
            // 可以选择struct pollfd 进行自动扩容！！这里是可以改的
        }
        else
        {
            __fds[pos].fd = sock; // 此时，我们就把accept到的sock放到数组里了，其他不用管了！
            __fds[pos].events = POLLIN;
        }
    }
    void Recver(int pos)
    {
        // 读事件就绪：INPUT事件到来，应该去recv，read
        logMessage(DEBUG, "message in, get IO event: %d",__fds[pos]);
        char buffer[1024];
        int n = recv(__fds[pos].fd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0)
        {
            buffer[n] = 0;
            logMessage(DEBUG, "client[%d]# %s", __fds[pos].fd, buffer);
        }
        else if(n == 0)
        {
            logMessage(DEBUG, "client[%d] quit, me too...", __fds[pos].fd);
            close(__fds[pos].fd);
            __fds[pos].fd = FD_NONE;
            __fds[pos].events = 0; // 不再需要关注了，清零一下，写的规范一点
        }
        else
        {
            logMessage(WARNING, "%d sock recv error, %d:%s", __fds[pos].fd, errno, strerror(errno));
            close(__fds[pos].fd);
            __fds[pos].fd = FD_NONE;
            __fds[pos].events = 0;
        }
    }
    void HandlerEvent()
    {
        for (int i = 0; i < __nfds; i++)
        {
            // 1. 去掉不合法的fd
            if (__fds[i].fd == FD_NONE)
                continue;
            // 2. 合法的就一定就绪了？不一定！
            if (__fds[i].revents & POLLIN) // 如果revents的POLLIN那一位设置成1了，就是IN事件就绪了！
            {
                if (__fds[i].fd == __listen_sock)
                {
                    Accepter();
                }
                else
                {
                   Recver(i);
                }
            }
        }
    }
    void debugPrint()
    {
        std::cout << "__fd_array[]:";
        for (int i = 0; i < __nfds; i++)
        {
            if (__fds[i].fd != FD_NONE)
                std::cout << __fds[i].fd << " ";
        }
        std::cout << std::endl;
    }
private:
    uint16_t __port;
    int __listen_sock;
    struct pollfd *__fds;
    nfds_t __nfds;
    int __timeout;
};
#endif