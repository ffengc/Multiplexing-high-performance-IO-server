
#ifndef __SELECT_SVR_H__
#define __SELECT_SVR_H__

#include <iostream>
#include <sys/select.h>
#include "Sock.hpp"
#include "Log.hpp"
#include <sys/time.h>
#include <string>
#include <vector>

#define BITS 8
#define NUM (sizeof(fd_set) * BITS)
#define FD_NONE -1 // 如果文件描输入不知道的时候，统一设置成-1

// select的服务器 我们只完成读取 写入和异常先不做处理，这些我们到epoll里面再写完整
class SelectServer
{
public:
    SelectServer(const uint16_t &port = 8080) : __port(port)
    {
        __listen_sock = Sock::Socket();
        Sock::Bind(__listen_sock, __port);
        Sock::Listen(__listen_sock);
        logMessage(DEBUG, "create base socket success");
        for (int i = 0; i < NUM; i++)
        {
            __fd_array[i] = FD_NONE;
        }
        // 规定 __fd_array[0] = __listen_sock --> 这个固定好就行，以后都不用动了，后面的再放后面接收的sock
        __fd_array[0] = __listen_sock;
    }
    ~SelectServer()
    {
        if (__listen_sock >= 0)
            close(__listen_sock);
    }
    void Start()
    {
        while (true)
        {
            // struct timeval timeout = {0, 0};
            // 如何看待listensock?
            // 获取新链接，我们依旧把它看作IO，input事件
            // 如果没有连接到来了呢？那就阻塞了！ -- 所以不能直接调用accept
            // int sock = Sock::Accept(__listen_sock, ...);
            // FD_SET(__listen_sock, &rfds); // 将listensocket添加到读文件描述符集里面
            this->debugPrint();
            fd_set rfds;    // 读文件描述符集
            FD_ZERO(&rfds); // 一定要保证一进来就要清空一次
            int maxfd = __listen_sock;
            for (int i = 0; i < NUM; i++)
            {
                if (__fd_array[i] == FD_NONE)
                    continue;
                FD_SET(__fd_array[i], &rfds); // 在数组里的都放进去
                if (maxfd < __fd_array[i])
                {
                    maxfd = __fd_array[i];
                }
            }

            int n = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
            // int n = select(__listen_sock + 1, &rfds, nullptr, nullptr, &timeout);
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
                logMessage(DEBUG, "get a new link event ..."); //
                // 有事件到来了，我们就要去处理这个事件啊，所以要告诉HandlerEvent哪一些事件就绪了
                HandlerEvent(rfds);
                break;
            }
        }
    }
private:
    void Accepter()
    {
        // 这个接口专门用来获取新连接，即：listensock就绪之后，HandlerEvent来调用Accepter！！
        std::string client_ip;
        uint16_t client_port;
        // listensock上面的读事件就绪了，表示可以读取了！
        // 可以获取新链接了
        int sock = Sock::Accept(__listen_sock, &client_ip, &client_port);
        // 这里进行accept会不会阻塞？一定不会！
        if (sock < 0)
        {
            logMessage(WARNING, "accept error");
            return;
        }
        logMessage(DEBUG, "get a new link success : [%s:%d] : %d", client_ip.c_str(), client_port, sock);
        // TODO
        // 能直接进行读取吗？不能！为什么不能？
        // 谁最清楚数据什么时候就绪呢？select
        // 得到新连接的时候，此时我们应该考虑的是，将新的sock脱光诶select，让select帮我们进行检测sock上是否有新的数据
        // 有了数据，select读事件就绪，select就会通知我，我们再进行读取，我们就不会被阻塞了！！
        // 要将sock添加给select，只需要将fd添加到我们的数组中即可！！
        int pos = 1;
        for (pos = 1; pos < NUM; pos++)
        {
            if (__fd_array[pos] == FD_NONE)
            {
                // 当前位置没有被占用
                break;
            }
        }
        if (pos == NUM)
        {
            // 数组满了，不能再加了，因为数组本来就是按照select监管的最大数量来定的
            logMessage(WARNING, "%s:%d", "select server already full, closs: %d", sock);
            close(sock);
        }
        else
        {
            __fd_array[pos] = sock; // 此时，我们就把accept到的sock放到数组里了，其他不用管了！
        }
    }
    void Recver(int pos) //这个pos方便等下如果sock关闭了，要对数组进行操作，所以要知道这个sock在数组中的位置
    {
        // 读事件就绪：INPUT事件到来，应该去recv，read
        logMessage(DEBUG, "message in, get IO event: %d", __fd_array[pos]);
        // 在这一次读取的时候，不会被阻塞！！！
        // 此时select已经进行了时间检测，即：本次不会被阻塞！！！
        char buffer[1024];
        int n = recv(__fd_array[pos], buffer, sizeof(buffer) - 1, 0);
        if (n > 0)
        {
            buffer[n] = 0;
            logMessage(DEBUG, "client[%d]# %s", __fd_array[pos], buffer);
        }
        else if(n == 0)
        {
            //对面把链接关了，之前讲过了
            logMessage(DEBUG, "client[%d] quit, me too...", __fd_array[pos]);
            //这里要做很多事情
            //1. 把这个sock关了
            //2. 从数组里面删了 -- 让select不再去关心这个sock
            close(__fd_array[pos]);
            __fd_array[pos] = FD_NONE;
        }
        else
        {
            logMessage(WARNING, "%d sock recv error, %d:%s", __fd_array[pos], errno, strerror(errno));
            close(__fd_array[pos]);
            __fd_array[pos] = FD_NONE;
        }
    }
    void HandlerEvent(const fd_set &rfds)
    {
        for (int i = 0; i < NUM; i++)
        {
            // 1. 去掉不合法的fd
            if (__fd_array[i] == FD_NONE)
                continue;
            // 2. 合法的就一定就绪了？不一定！
            if (FD_ISSET(__fd_array[i], &rfds))
            {
                if (__fd_array[i] == __listen_sock)
                {
                    // 读事件就绪：连接事件到来，应该去accept
                    Accepter(); // 去accept
                }
                else
                {
                   Recver(i); // 传位置就行
                }
            }
        }
    }
    void debugPrint()
    {
        std::cout << "__fd_array[]:";
        for (int i = 0; i < NUM; i++)
        {
            if (__fd_array[i] != -1)
                std::cout << __fd_array[i] << " ";
        }
        std::cout << std::endl;
    }
private:
    uint16_t __port;
    int __listen_sock;
    int __fd_array[NUM]; // 第三方数组
};
#endif