#include "ScoliosisQueryNN.h"
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

NeuralNetworkSocketConnection::NeuralNetworkSocketConnection():
	m_socket(io_context){
    tcp::resolver resolver(io_context);
    boost::asio::connect(m_socket, resolver.resolve("hastings-alien.local", "9999"));
}

double NeuralNetworkSocketConnection::QueryNN(unsigned char * image) {
  boost::asio::write(this->m_socket, boost::asio::buffer(image, 512 * 512));
  unsigned char reply[4];
  size_t reply_length = boost::asio::read(this->m_socket, boost::asio::buffer(reply, 4));
    
  return * (reinterpret_cast<float * >( &reply[0] ));
}