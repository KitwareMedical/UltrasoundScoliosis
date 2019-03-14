#ifndef _ScoliosisQueryNN_h
#define _ScoliosisQueryNN_h
#include <boost/asio.hpp>

using boost::asio::ip::tcp;



class NeuralNetworkSocketConnection {
public:
    NeuralNetworkSocketConnection();
    double QueryNN(unsigned char * image);
private:
    boost::asio::io_context io_context;
    tcp::socket m_socket;
};

#endif //_ScoliosisQueryNN_h