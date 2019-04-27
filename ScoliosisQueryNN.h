#ifndef _ScoliosisQueryNN_h
#define _ScoliosisQueryNN_h
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

struct NetworkResponse {
    float angleInRadians;
    float stdDevInRadians;
};

class NeuralNetworkSocketConnection {
public:
    NeuralNetworkSocketConnection();
    bool Connect();
    NetworkResponse QueryNN(unsigned char * image);
private:
    bool connected;
    boost::asio::io_context io_context;
    tcp::socket m_socket;
};

#endif //_ScoliosisQueryNN_h