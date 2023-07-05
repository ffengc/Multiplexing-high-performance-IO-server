

#include "selectServer.hpp"
#include <memory>

int main()
{
    std::unique_ptr<SelectServer> svr(new SelectServer());
    svr->Start();
    return 0;
}