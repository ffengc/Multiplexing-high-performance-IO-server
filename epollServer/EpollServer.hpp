#ifndef __EPOLL_SERVER_HPP__
#define __EPOLL_SERVER_HPP__

#include <unistd.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <functional>
#include "Log.hpp"
#include "Sock.hpp"
#include "Epoll.hpp"


// 只处理读取
namespace ns_epoll
{
    static const int __default_port = 8080;
    static const int __gnum = 64;
    class EpollServer
    {
    public:
        using func_t  = std::function<void(std::string)>;
    private:
        int __listen_sock;
        int __epfd; // 表示创建好的epoll模型
        uint16_t __port;
        struct epoll_event *__revs; // 就绪的事件可以都存到这里来
        int __revs_num;
        func_t __handler_request;
    private:
        void __accepter(int listen_sock)
        {
            std::string client_ip;
            uint16_t client_port;
            int sock = Sock::Accept(listen_sock, &client_ip, &client_port);
            if(sock < 0)
            {
                logMessage(WARNING, "accept error!");
                return;
            }
            // 能不能直接读取？拿到这个sock之后，丢给epoll！
            if(!Epoll::CtlEpoll(__epfd, EPOLL_CTL_ADD, sock, EPOLLIN)) return;
            logMessage(DEBUG, "add new sock : %d to epoll success", sock);
        }
        void __recver(int sock)
        {
            // 1. 读取数据
            // 2. 解决黏包问题等（我们这里只做简单处理）
            char buffer[10240];
            ssize_t n = recv(sock, buffer, sizeof(buffer)-1, 0);
            if(n > 0)
            {
                //假设这里读到了一个完整的报文
                //其实是要定制协议的，这里先假设读到了完整报文
                buffer[n] = 0;
                __handler_request(buffer);
            }
            else if(n == 0)
            {
                //从epoll中去掉，再关
                bool if_success = Epoll::CtlEpoll(__epfd, EPOLL_CTL_DEL, sock, 0); // 去掉
                assert(if_success);
                (void)if_success;
                //对面把链接关了
                close(sock);
                logMessage(NORMAL, "client %d quit, me too...", sock);
            }
            else
            {
                //读取出错，和上面=0的动作是一样的
                bool if_success = Epoll::CtlEpoll(__epfd, EPOLL_CTL_DEL, sock, 0); // 去掉
                assert(if_success);
                (void)if_success;
                close(sock);
                logMessage(WARNING, "recv error, I quit", sock);
            }
        }
        void __handler_event(int n)
        {
            assert(n > 0);
            for (int i = 0; i < n; i++)
            {
                uint32_t revents = __revs[i].events;
                int sock = __revs[i].data.fd;
                if (revents & EPOLLIN)
                {
                    // 读事件就绪
                    // 1. listen_sock就绪 2. 一般的sock就绪
                    if (sock == __listen_sock)
                        __accepter(__listen_sock); // 把listen套接字传进去，直接recv
                    else
                        __recver(sock); // 把就绪的sock传进去
                }
            }
        }
        void __loop_once(int timeout)
        {
            int n = Epoll::WaitEpoll(__epfd, __revs, __revs_num, timeout);
            switch (n)
            {
            case 0:
                // logMessage(DEBUG, "timeout..."); 等不到可以不打印了
                break;
            case -1:
                logMessage(WARNING, "epoll wait error: %s", strerror(errno));
                break;
            default:
                // 成功取到就绪的事件了，等待成功
                logMessage(DEBUG, "get a event, event num: %d", n);
                __handler_event(n); // 要告诉我要处理几个就行了！
                break;
            }
        }

    public:
        EpollServer(func_t handler_request, const int &port = __default_port) 
                : __port(port), __revs_num(__gnum), __handler_request(handler_request)
        {
            // 0. 申请空间
            __revs = new struct epoll_event[__revs_num];
            // 1. 创建listensock
            __listen_sock = Sock::Socket();
            Sock::Bind(__listen_sock, __port);
            Sock::Listen(__listen_sock);
            // 2. 创建epoll模型 -> 这个也可以封装一下
            __epfd = Epoll::CreateEpoll();
            logMessage(DEBUG, "init sucess, listensock: %d, epfd: %d", __listen_sock, __epfd);
            // 3. 将listensock先添加到epoll中，让epoll帮我们管理起来！ --  封装一下
            bool if_add_success = Epoll::CtlEpoll(__epfd, EPOLL_CTL_ADD, __listen_sock, EPOLLIN);
            if (if_add_success)
                logMessage(DEBUG, "add listen_sock to epoll success");
            else
                exit(6); // 添加失败直接结束
        }
        ~EpollServer()
        {
            if (__listen_sock >= 0)
                close(__listen_sock);
            if (__epfd >= 0)
                close(__epfd);
            if (__revs)
                delete[] __revs;
        }
        void start()
        {
            int timeout = 1000;
            while (true)
            {
                this->__loop_once(timeout);
            }
        }
    };
}

#endif