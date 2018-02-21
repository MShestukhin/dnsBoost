
#include <boost/asio.hpp>
#include <iostream>
using namespace std;
namespace asio=boost::asio;
int main()
{
    //QCoreApplication a(argc, argv);

    try{
        asio::io_service service;
        asio::ip::tcp::resolver resolver(service);
        asio::ip::tcp::resolver::query query("google.com", "53");
        asio::ip::tcp::resolver::iterator end, iter=resolver.resolve(query);
        while(iter!=end){
            asio::ip::tcp::endpoint endpoint= iter->endpoint();
            std::cout<<endpoint.address().to_string().c_str()<<"\n";
            iter++;
        }
    }
    catch(std::exception &ex){
        std::cout<<ex.what()<<std::endl;
    }
    return 0;
    //return a.exec();
}
