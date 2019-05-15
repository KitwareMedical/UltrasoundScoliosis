#include "server_http.hpp"
#include "ScoliosisServer.h"


#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>



using namespace std;
using namespace boost::property_tree;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

HttpServer server;

double server_roll = 0;

int start_server() {
  server.config.port = 80;

  server.resource["^/transform"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
	  try {
		  ptree pt;
		  read_json(request->content, pt);
		  
		  int first = 1;
		  for (auto& item : pt.get_child("")) {
			  if (first == 1) {
				  server_roll = item.second.get<double>("");
			  }
			  first = 0;
			  std::cout << item.second.get<double>("") << " ";
		  }
		  std::cout << std::endl;
		  response->write("");
	  }
	  catch (const exception &e) {
		  *response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n"
			  << e.what();
	  }
  };
 
  server.resource["^/time$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
	  std::chrono::milliseconds time = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now().time_since_epoch());

	  response->write(std::to_string(time.count() / 1000.));
  };

  server.on_error = [](shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/) {
    // Handle errors here
    // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
  };

  HttpServer * serverptr = &server;
  thread server_thread([serverptr]() {
    // Start server
    server.start();
  });

  server_thread.detach();
  return 0;
}
