
#include <boost/asio.hpp>
#include <dnsclient.h>

using namespace std;
namespace asio=boost::asio;
int main()
{

    boost::asio::io_service io_service;
    DnsClient* dns=new DnsClient(io_service);
    char hostname[100];
    scanf("%s",hostname);
    dns->do_send(hostname);
    io_service.run();
    return 0;
}
