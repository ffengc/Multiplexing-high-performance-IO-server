

#include <poll.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>

int main()
{
    struct pollfd poll_fd;
    poll_fd.fd = 0;
    poll_fd.events = POLLIN;

    for (;;)
    {
        int ret = poll(&poll_fd, 1, 1000);
        if (ret < 0)
        {
            perror("poll");
            continue;
        }
        if (ret == 0)
        {
            printf("poll timeout\n");
            continue;
        }
        if (poll_fd.revents == POLLIN)
        {
            char buf[1024] = {0};
            read(0, buf, sizeof(buf) - 1);
            printf("stdin: %s", buf);
        }
    }
    return 0;
}