

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#if false
bool SetNonBlock(int fd)
{
    int fl = fcntl(fd, F_GETFL); // 在底层获取当前fd对应文件的读写标记位
    if (fl < 0)
        return false;
    fcntl(fd, F_SETFL, fl | O_NONBLOCK); // 设置非阻塞
    return true;
}
int main()
{
    // stdin -- 0
    // 0号描述符就是很好的用来测试的描述符

    SetNonBlock(0);

    char buffer[1024];
    while (true)
    {
        sleep(1);
        errno = 0;
        ssize_t s = read(0, buffer, sizeof(buffer) - 1); // 出错，不仅仅是错误返回值，errno变量也会被设置，表明出错原因
        if (s > 0)
        {
            buffer[s - 1] = 0;
            std::cout << "echo# " << buffer << " errno: " << errno << " errstring: " << strerror(errno) << std::endl;
        }
        else
        {
            // TODO
            std::cout << "read \"error\""
                      << " errno: " << errno << " errstring: " << strerror(errno) << std::endl;
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                std::cout << "当前0号fd数据没有就绪, 请下一次再来试试吧" << std::endl;
                continue;
            }
            else if (errno == EINTR)
            {
                std::cout << "当前IO被信号中断了, 再试一试吧" << std::endl;
                continue;
            }
            else
            {
                //出错处理
            }
        }
    }
    return 0;
}
#endif

// ---------------------------------------- 学习select ----------------------------------------//

// 认识time
int main()
{
    while(true)
    {
        //用C语言的方式进行获取
        std::cout << "time: " << (unsigned long)time(nullptr) << std::endl;
        //用系统调用的方式进行获取
        struct timeval currtime = {0,0};
        int n = gettimeofday(&currtime, nullptr);
        (void)n;
        std::cout << "gettimeofday: " << currtime.tv_sec << "." << currtime.tv_usec << std::endl;
        sleep(1);
    }
    return 0;
}