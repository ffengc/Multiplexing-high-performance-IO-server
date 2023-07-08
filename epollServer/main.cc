

#include "EpollServer.hpp"
#include <memory>

using namespace ns_epoll;


void change(std::string request)
{
    // 完成业务逻辑
    std::cout << "[I get a string]: " << request << std::endl; 
}

int main()
{
    std::unique_ptr<EpollServer> epoll_server(new EpollServer(change));
    epoll_server->start();
    return 0;
}