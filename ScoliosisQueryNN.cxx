#include "ScoliosisQueryNN.h"
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

NeuralNetworkSocketConnection::NeuralNetworkSocketConnection():
	m_socket(io_context),
	connected(false){
	Connect();
    
}

bool NeuralNetworkSocketConnection::Connect() {

	if (connected) return true;
	try
	{
		tcp::resolver resolver(io_context);
		boost::asio::connect(m_socket, resolver.resolve("localhost", "9999"));
		connected = true;
		return true;
	} 
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return false;
	}
}

NetworkResponse NeuralNetworkSocketConnection::QueryNN(unsigned char * image) {
  boost::asio::write(this->m_socket, boost::asio::buffer(image, 512 * 512));
  unsigned char reply[8];
  size_t reply_length = boost::asio::read(this->m_socket, boost::asio::buffer(reply, 8));
  float * reply_cast = reinterpret_cast<float *>(&reply[0]);
  return NetworkResponse{ reply_cast[0], reply_cast[1] };
}